#!/usr/bin/env python3
"""Build a local OGG cache from the user's legally obtained Tetris NES ROM.

The repository never contains the generated music, effects, WAV files or ROM.
The existing 6502/APU renderer produces PCM locally and ffmpeg encodes it as
Ogg Vorbis. The PC port automatically looks for the resulting directory in
its SDL preference path, beside the executable, or through --audio-pack.

Music is rendered long enough to observe two complete cycles of the original
ROM driver. The generated APU trace is then used to locate the exact frame and
PCM-sample boundaries of the introduction and loop. Looping OGG files receive
LOOPSTART and LOOPEND Vorbis comments understood by SDL_mixer.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable

SCHEMA = "tetris-nes-rom-audio-cache-v2"
NTSC_CPU_CLOCK = 1_789_773
AUDIO_SAMPLE_RATE = 48_000
LOOP_WINDOW_FRAMES = 120
MIN_LOOP_PERIOD_FRAMES = 120
STABLE_TAIL_FRAMES = 120
IGNORED_LOOP_WRITE_ADDRESSES = {0x4000, 0x4004, 0x400C, 0x4017}
TRACKS = tuple(range(1, 11))
EFFECTS = (
    ("move", "move.ogg"),
    ("rotate", "rotate.ogg"),
    ("lock", "lock.ogg"),
    ("line", "line.ogg"),
    ("tetris", "tetris.ogg"),
    ("level", "level_up.ogg"),
    ("game-over", "game_over.ogg"),
    ("complete", "complete.ogg"),
)
# The port's three user-selectable songs correspond to original driver tracks
# 3, 4 and 5. All ten tracks are still retained as track_01..track_10.
PLAYABLE_ALIASES = {
    "music_1.ogg": "track_03.ogg",
    "music_2.ogg": "track_04.ogg",
    "music_3.ogg": "track_05.ogg",
}


def default_output_dir() -> Path:
    system = platform.system()
    home = Path.home()
    if system == "Windows":
        root = Path(os.environ.get("APPDATA", home / "AppData" / "Roaming"))
        return root / "YlPorts" / "TetrisNESPC" / "audio"
    if system == "Darwin":
        return (
            home
            / "Library"
            / "Application Support"
            / "YlPorts"
            / "TetrisNESPC"
            / "audio"
        )
    root = Path(os.environ.get("XDG_DATA_HOME", home / ".local" / "share"))
    return root / "YlPorts" / "TetrisNESPC" / "audio"


def executable_name(base: str) -> str:
    return f"{base}.exe" if os.name == "nt" else base


def find_executable(base: str, bin_dir: Path | None) -> Path:
    name = executable_name(base)
    candidates: list[Path] = []
    if bin_dir:
        candidates.extend((bin_dir / name, bin_dir / "Release" / name))
    repository = Path(__file__).resolve().parents[1]
    candidates.extend(
        (
            Path.cwd() / name,
            Path.cwd() / "build" / name,
            Path.cwd() / "build" / "Release" / name,
            repository / "build" / name,
            repository / "build" / "Release" / name,
        )
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    located = shutil.which(name)
    if located:
        return Path(located).resolve()
    raise FileNotFoundError(
        f"No se encontró {name}. Compila el proyecto o usa --bin-dir."
    )


def run(command: Iterable[os.PathLike[str] | str]) -> str:
    rendered = [str(part) for part in command]
    print("+", " ".join(rendered), flush=True)
    completed = subprocess.run(
        rendered,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if completed.stdout:
        print(completed.stdout, end="" if completed.stdout.endswith("\n") else "\n")
    if completed.returncode != 0:
        raise RuntimeError(
            f"El comando terminó con código {completed.returncode}: {rendered[0]}"
        )
    return completed.stdout


def encode_ogg(
    ffmpeg: Path,
    wav: Path,
    output: Path,
    quality: int,
    *,
    end_sample: int,
    loop_start_sample: int | None = None,
    loop_end_sample: int | None = None,
    title: str | None = None,
) -> None:
    if end_sample <= 0:
        raise RuntimeError(f"Duración PCM inválida para {output.name}: {end_sample}")
    if (loop_start_sample is None) != (loop_end_sample is None):
        raise RuntimeError("LOOPSTART y LOOPEND deben proporcionarse juntos")
    if loop_start_sample is not None and not (
        0 <= loop_start_sample < loop_end_sample <= end_sample
    ):
        raise RuntimeError(
            f"Puntos de bucle inválidos para {output.name}: "
            f"{loop_start_sample}..{loop_end_sample} de {end_sample}"
        )

    command: list[os.PathLike[str] | str] = [
        ffmpeg,
        "-hide_banner",
        "-loglevel",
        "error",
        "-y",
        "-i",
        wav,
        "-vn",
        "-map_metadata",
        "-1",
        "-af",
        f"atrim=end_sample={end_sample},asetpts=PTS-STARTPTS",
        "-ac",
        "1",
        "-ar",
        str(AUDIO_SAMPLE_RATE),
        "-c:a",
        "libvorbis",
        "-q:a",
        str(quality),
    ]
    if title:
        command.extend(("-metadata", f"TITLE={title}"))
    if loop_start_sample is not None and loop_end_sample is not None:
        # SDL_mixer 2.6+ reads these Vorbis comments as exact decoded-sample
        # positions and seeks to LOOPSTART when LOOPEND is reached.
        command.extend(("-metadata", f"LOOPSTART={loop_start_sample}"))
        command.extend(("-metadata", f"LOOPEND={loop_end_sample}"))
    command.append(output)

    output.parent.mkdir(parents=True, exist_ok=True)
    run(command)
    if not output.is_file() or output.stat().st_size < 128:
        raise RuntimeError(f"ffmpeg no creó un OGG válido: {output}")


def parse_key_values(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for raw_line in text.splitlines():
        if "=" not in raw_line:
            continue
        key, value = raw_line.split("=", 1)
        if key and key.replace("_", "").isalnum():
            result[key] = value
    return result


def normalize_loop_writes(serialized: str) -> str:
    kept: list[str] = []
    for raw_write in serialized.split("|"):
        if not raw_write or "=" not in raw_write:
            continue
        address_text, _ = raw_write.split("=", 1)
        try:
            address = int(address_text, 16)
        except ValueError:
            continue
        # The volume/envelope registers vary as a consequence of the current
        # note and hide the repeated bytecode sequence. The remaining writes
        # preserve note, instrument, sweep, pitch, noise and DMC events.
        if address not in IGNORED_LOOP_WRITE_ADDRESSES:
            kept.append(raw_write)
    return "|".join(kept)


def samples_before_frame(rows: list[dict[str, str]], frame: int) -> int:
    if frame < 0 or frame > len(rows):
        raise RuntimeError(f"Índice de frame fuera de rango: {frame}")
    cycles = sum(int(row["cpu_cycles"]) for row in rows[:frame])
    return cycles * AUDIO_SAMPLE_RATE // NTSC_CPU_CLOCK


def analyze_music_trace(trace: Path) -> dict[str, int | bool]:
    with trace.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    if len(rows) < STABLE_TAIL_FRAMES * 2:
        raise RuntimeError(
            f"Traza demasiado corta para detectar bucles exactos: {trace}"
        )
    required = {"cpu_cycles", "apu_writes"}
    if not rows or not required.issubset(rows[0]):
        raise RuntimeError(f"Formato de traza incompatible: {trace}")

    signatures = [normalize_loop_writes(row["apu_writes"]) for row in rows]

    # Tracks 1 and 2 are one-shot. Once their script has ended, only the
    # ignored frame-counter/volume maintenance remains, producing a stable
    # normalized tail. Trim that silence instead of inventing a loop.
    stable_start = len(signatures) - 1
    while stable_start > 0 and signatures[stable_start - 1] == signatures[-1]:
        stable_start -= 1
    if len(signatures) - stable_start >= STABLE_TAIL_FRAMES:
        end_sample = samples_before_frame(rows, stable_start)
        return {
            "loops": False,
            "end_frame": stable_start,
            "end_sample": end_sample,
        }

    seen: dict[tuple[str, ...], list[int]] = {}
    candidates: list[tuple[int, int]] = []
    count = len(signatures)
    for index in range(0, count - LOOP_WINDOW_FRAMES + 1):
        key = tuple(signatures[index:index + LOOP_WINDOW_FRAMES])
        previous = seen.get(key)
        if previous:
            for start in previous:
                period = index - start
                if period < MIN_LOOP_PERIOD_FRAMES or period & 1:
                    continue
                if index + period > count:
                    continue
                # Require two complete, identical periods. This rejects a
                # repeated phrase unless the bytecode-generated event stream
                # truly cycles from the same frame boundary.
                if signatures[start:index] == signatures[index:index + period]:
                    candidates.append((start, period))
            previous.append(index)
        else:
            seen[key] = [index]

    if not candidates:
        raise RuntimeError(
            f"No se encontró un ciclo verificable en {trace}. "
            "Aumenta --music-seconds."
        )

    start_frame, period_frames = min(candidates)
    end_frame = start_frame + period_frames
    loop_start_sample = samples_before_frame(rows, start_frame)
    loop_end_sample = samples_before_frame(rows, end_frame)
    if loop_end_sample <= loop_start_sample:
        raise RuntimeError(f"Bucle PCM vacío detectado en {trace}")
    return {
        "loops": True,
        "loop_start_frame": start_frame,
        "loop_end_frame": end_frame,
        "loop_frames": period_frames,
        "loop_start_sample": loop_start_sample,
        "loop_end_sample": loop_end_sample,
        "end_sample": loop_end_sample,
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def expected_files() -> set[str]:
    names = {f"track_{track:02d}.ogg" for track in TRACKS}
    names.update(filename for _, filename in EFFECTS)
    names.update(PLAYABLE_ALIASES)
    names.add("audio-cache.json")
    return names


def self_test() -> int:
    expected = expected_files()
    assert len(TRACKS) == 10
    assert len(EFFECTS) == 8
    assert len(expected) == 22
    assert set(PLAYABLE_ALIASES.values()).issubset(
        {f"track_{track:02d}.ogg" for track in TRACKS}
    )
    assert default_output_dir().name == "audio"
    assert normalize_loop_writes(
        "4017=C0|4000=F1|4002=34|4003=08|400C=10|400E=03"
    ) == "4002=34|4003=08|400E=03"

    with tempfile.TemporaryDirectory(prefix="tetris-loop-self-test-") as directory:
        root = Path(directory)
        looping = root / "loop.csv"
        period = [
            f"4002={index & 0xFF:02X}|4003={(index >> 8) & 0xFF:02X}"
            for index in range(140)
        ]
        signatures = [f"4006={index:02X}" for index in range(10)]
        signatures.extend(period * 3)
        with looping.open("w", newline="", encoding="utf-8") as output:
            writer = csv.writer(output)
            writer.writerow(
                (
                    "frame",
                    "cpu_cycles",
                    "driver_cycles",
                    "dmc_stall_cycles",
                    "irq",
                    "apu_writes",
                )
            )
            for frame, writes in enumerate(signatures):
                writer.writerow((frame, 29780 + (frame & 1), 1000, 0, 0, writes))
        loop = analyze_music_trace(looping)
        assert loop["loops"] is True
        assert loop["loop_start_frame"] == 10
        assert loop["loop_frames"] == 140

        one_shot = root / "one-shot.csv"
        signatures = [f"4002={index:02X}" for index in range(10)] + [""] * 240
        with one_shot.open("w", newline="", encoding="utf-8") as output:
            writer = csv.writer(output)
            writer.writerow(
                (
                    "frame",
                    "cpu_cycles",
                    "driver_cycles",
                    "dmc_stall_cycles",
                    "irq",
                    "apu_writes",
                )
            )
            for frame, writes in enumerate(signatures):
                writer.writerow((frame, 29780 + (frame & 1), 1000, 0, 0, writes))
        ending = analyze_music_trace(one_shot)
        assert ending["loops"] is False
        assert ending["end_frame"] == 10

    print(
        "audio cache self-test: OK tracks=10 effects=8 aliases=3 "
        "files=22 loop-tags=sample-exact"
    )
    return 0


def build(args: argparse.Namespace) -> int:
    rom = args.rom.expanduser().resolve()
    if not rom.is_file():
        raise FileNotFoundError(f"No existe la ROM: {rom}")

    output = args.output.expanduser().resolve()
    output.mkdir(parents=True, exist_ok=True)
    manifest_path = output / "audio-cache.json"
    if manifest_path.exists() and not args.overwrite:
        raise FileExistsError(
            f"Ya existe {manifest_path}. Usa --overwrite para regenerarlo."
        )

    bin_dir = args.bin_dir.expanduser().resolve() if args.bin_dir else None
    renderer = find_executable("tetris_apu_render", bin_dir)
    scenario_renderer = find_executable("tetris_apu_scenario", bin_dir)
    ffmpeg_string = str(args.ffmpeg) if args.ffmpeg else shutil.which("ffmpeg")
    if not ffmpeg_string:
        raise FileNotFoundError(
            "No se encontró ffmpeg. Instálalo o especifica --ffmpeg."
        )
    ffmpeg = Path(ffmpeg_string).expanduser().resolve()
    if not ffmpeg.is_file():
        raise FileNotFoundError(f"No existe ffmpeg: {ffmpeg}")

    rom_sha256 = sha256_file(rom)
    rom_crc32 = ""
    track_records: list[dict[str, object]] = []
    effect_records: list[dict[str, object]] = []

    with tempfile.TemporaryDirectory(prefix="tetris-audio-cache-") as temporary:
        temp = Path(temporary)
        for track in TRACKS:
            stem = f"track_{track:02d}"
            wav = temp / f"{stem}.wav"
            trace = output / f"{stem}.csv"
            ogg = output / f"{stem}.ogg"
            result = run(
                (
                    renderer,
                    rom,
                    str(track),
                    str(args.music_seconds),
                    wav,
                    trace,
                )
            )
            values = parse_key_values(result)
            if not rom_crc32:
                rom_crc32 = values.get("ROM_CRC32", "")
            loop = analyze_music_trace(trace)
            loop_start = int(loop["loop_start_sample"]) if loop["loops"] else None
            loop_end = int(loop["loop_end_sample"]) if loop["loops"] else None
            end_sample = int(loop["end_sample"])
            encode_ogg(
                ffmpeg,
                wav,
                ogg,
                args.quality,
                end_sample=end_sample,
                loop_start_sample=loop_start,
                loop_end_sample=loop_end,
                title=f"Tetris NES driver track {track:02d}",
            )
            if args.keep_wav:
                shutil.copy2(wav, output / wav.name)
            record: dict[str, object] = {
                "driver_track": track,
                "file": ogg.name,
                "trace": trace.name,
                "seconds_requested": args.music_seconds,
                "rendered_samples": int(values.get("SAMPLES", "0")),
                "encoded_samples": end_sample,
                "apu_write_hash": values.get("APU_WRITE_HASH", ""),
                "loops": bool(loop["loops"]),
            }
            if loop["loops"]:
                record["loop_start_frame"] = int(loop["loop_start_frame"])
                record["loop_end_frame"] = int(loop["loop_end_frame"])
                record["loop_frames"] = int(loop["loop_frames"])
                record["loop_start_sample"] = loop_start
                record["loop_end_sample"] = loop_end
            else:
                record["one_shot_end_frame"] = int(loop["end_frame"])
            track_records.append(record)

        for scenario, filename in EFFECTS:
            stem = Path(filename).stem
            wav = temp / f"{stem}.wav"
            trace = output / f"effect_{stem}.csv"
            ogg = output / filename
            result = run(
                (
                    scenario_renderer,
                    rom,
                    scenario,
                    str(args.effect_frames),
                    trace,
                    wav,
                    "--isolated",
                )
            )
            values = parse_key_values(result)
            encode_ogg(
                ffmpeg,
                wav,
                ogg,
                args.quality,
                end_sample=int(values.get("SAMPLES", "0")),
                title=f"Tetris NES effect: {scenario}",
            )
            if args.keep_wav:
                shutil.copy2(wav, output / wav.name)
            effect_records.append(
                {
                    "scenario": scenario,
                    "file": filename,
                    "trace": trace.name,
                    "frames": args.effect_frames,
                    "samples": int(values.get("SAMPLES", "0")),
                }
            )

    for alias, source_name in PLAYABLE_ALIASES.items():
        shutil.copy2(output / source_name, output / alias)

    manifest = {
        "schema": SCHEMA,
        "generator_version": "0.28",
        "rom": {
            "filename": rom.name,
            "sha256": rom_sha256,
            "renderer_crc32": rom_crc32,
        },
        "format": {
            "container": "ogg",
            "codec": "vorbis",
            "sample_rate": AUDIO_SAMPLE_RATE,
            "channels": 1,
            "quality": args.quality,
        },
        "tracks": track_records,
        "effects": effect_records,
        "playable_aliases": PLAYABLE_ALIASES,
        "notes": [
            "Generated locally from the user's legal ROM; never commit this directory.",
            "Tracks 3-10 carry LOOPSTART/LOOPEND Vorbis comments derived from repeated ROM-driver event cycles.",
            "Tracks 1 and 2 are one-shot and are trimmed at the first stable post-script frame.",
            "SDL_mixer 2.6+ honors the embedded sample-exact loop points automatically.",
        ],
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    missing = sorted(expected_files() - {path.name for path in output.iterdir()})
    if missing:
        raise RuntimeError(f"Faltan archivos del paquete: {', '.join(missing)}")

    print(f"\nPaquete OGG completo: {output}")
    print("Pistas originales: 10")
    print("Efectos aislados: 8")
    print(f"ROM SHA-256: {rom_sha256}")
    print(
        "El port lo cargará automáticamente si está en su carpeta "
        "de preferencias/audio."
    )
    return 0


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description=(
            "Renderiza las pistas y efectos originales de la ROM legal a un "
            "caché OGG local con puntos de bucle exactos."
        )
    )
    result.add_argument("--self-test", action="store_true")
    result.add_argument("--rom", type=Path)
    result.add_argument("--bin-dir", type=Path)
    result.add_argument("--output", type=Path, default=default_output_dir())
    result.add_argument("--ffmpeg", type=Path)
    result.add_argument("--music-seconds", type=int, default=180)
    result.add_argument("--effect-frames", type=int, default=240)
    result.add_argument("--quality", type=int, default=5)
    result.add_argument("--keep-wav", action="store_true")
    result.add_argument("--overwrite", action="store_true")
    return result


def main() -> int:
    args = parser().parse_args()
    if args.self_test:
        return self_test()
    if args.rom is None:
        parser().error("--rom es obligatorio salvo con --self-test")
    if not 120 <= args.music_seconds <= 3600:
        parser().error(
            "--music-seconds debe estar entre 120 y 3600 para detectar dos ciclos"
        )
    if not 30 <= args.effect_frames <= 36000:
        parser().error("--effect-frames debe estar entre 30 y 36000")
    if not 0 <= args.quality <= 10:
        parser().error("--quality debe estar entre 0 y 10")
    try:
        return build(args)
    except (FileNotFoundError, FileExistsError, RuntimeError, OSError) as error:
        print(f"audio cache error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
