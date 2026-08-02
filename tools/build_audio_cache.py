#!/usr/bin/env python3
"""Build a local OGG cache from the user's legally obtained Tetris NES ROM.

The repository never contains the generated music, effects, WAV files or ROM.
The existing 6502/APU renderer produces PCM locally and ffmpeg encodes it as
Ogg Vorbis.  The PC port automatically looks for the resulting directory in
its SDL preference path, beside the executable, or through --audio-pack.
"""

from __future__ import annotations

import argparse
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

SCHEMA = "tetris-nes-rom-audio-cache-v1"
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
# 3, 4 and 5.  All ten tracks are still retained as track_01..track_10.
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
        return home / "Library" / "Application Support" / "YlPorts" / "TetrisNESPC" / "audio"
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


def encode_ogg(ffmpeg: Path, wav: Path, output: Path, quality: int) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    run(
        (
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
            "-ac",
            "1",
            "-ar",
            "48000",
            "-c:a",
            "libvorbis",
            "-q:a",
            str(quality),
            output,
        )
    )
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
    print("audio cache self-test: OK tracks=10 effects=8 aliases=3 files=22")
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
            encode_ogg(ffmpeg, wav, ogg, args.quality)
            if args.keep_wav:
                shutil.copy2(wav, output / wav.name)
            track_records.append(
                {
                    "driver_track": track,
                    "file": ogg.name,
                    "trace": trace.name,
                    "seconds_requested": args.music_seconds,
                    "samples": int(values.get("SAMPLES", "0")),
                    "apu_write_hash": values.get("APU_WRITE_HASH", ""),
                }
            )

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
            encode_ogg(ffmpeg, wav, ogg, args.quality)
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
        "generator_version": "0.20",
        "rom": {
            "filename": rom.name,
            "sha256": rom_sha256,
            "renderer_crc32": rom_crc32,
        },
        "format": {
            "container": "ogg",
            "codec": "vorbis",
            "sample_rate": 48000,
            "channels": 1,
            "quality": args.quality,
        },
        "tracks": track_records,
        "effects": effect_records,
        "playable_aliases": PLAYABLE_ALIASES,
        "notes": [
            "Generated locally from the user's legal ROM; never commit this directory.",
            "The current runtime uses music_1.ogg through music_3.ogg and all eight effect files.",
            "track_01.ogg through track_10.ogg are retained for continued ROM correspondence work.",
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
    print("El port lo cargará automáticamente si está en su carpeta de preferencias/audio.")
    return 0


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description="Renderiza las pistas y efectos originales de la ROM legal a un caché OGG local."
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
    if not 10 <= args.music_seconds <= 3600:
        parser().error("--music-seconds debe estar entre 10 y 3600")
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
