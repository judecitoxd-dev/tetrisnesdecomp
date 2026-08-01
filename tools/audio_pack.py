#!/usr/bin/env python3
"""Build or validate an optional Ogg Vorbis audio pack for the PC port.

This tool does not extract or distribute copyrighted audio. It converts audio
files that the user supplied, for example captures rendered from their own
legal cartridge dump in an emulator or APU tool.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

MUSIC_FILES = ("music_1", "music_2", "music_3")
SFX_FILES = (
    "move",
    "rotate",
    "lock",
    "line",
    "tetris",
    "level_up",
    "game_over",
    "complete",
)
ALL_FILES = MUSIC_FILES + SFX_FILES
SOURCE_EXTENSIONS = (".wav", ".flac", ".ogg", ".oga", ".mp3", ".m4a")


def find_source(directory: Path, stem: str) -> Path | None:
    for extension in SOURCE_EXTENSIONS:
        candidate = directory / f"{stem}{extension}"
        if candidate.is_file():
            return candidate
    return None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run_ffmpeg(ffmpeg: str, source: Path, destination: Path, music: bool) -> None:
    channels = "2" if music else "1"
    quality = "5" if music else "6"
    command = [
        ffmpeg,
        "-hide_banner",
        "-loglevel",
        "error",
        "-y",
        "-i",
        str(source),
        "-vn",
        "-map_metadata",
        "-1",
        "-ar",
        "48000",
        "-ac",
        channels,
        "-c:a",
        "libvorbis",
        "-q:a",
        quality,
        str(destination),
    ]
    result = subprocess.run(command, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"ffmpeg failed while converting {source.name}")


def validate_pack(directory: Path) -> tuple[bool, list[str]]:
    problems: list[str] = []
    for stem in ALL_FILES:
        path = directory / f"{stem}.ogg"
        if not path.is_file():
            problems.append(f"missing {path.name}")
        elif path.stat().st_size < 64:
            problems.append(f"invalid or empty {path.name}")
    return not problems, problems


def build_pack(source_dir: Path, output_dir: Path, ffmpeg: str) -> None:
    sources: dict[str, Path] = {}
    missing: list[str] = []
    for stem in ALL_FILES:
        source = find_source(source_dir, stem)
        if source is None:
            missing.append(stem)
        else:
            sources[stem] = source
    if missing:
        extensions = ", ".join(SOURCE_EXTENSIONS)
        raise RuntimeError(
            "missing source files: " + ", ".join(missing) +
            f" (accepted extensions: {extensions})"
        )

    output_dir.mkdir(parents=True, exist_ok=True)
    manifest: dict[str, object] = {
        "format": "tetris-nes-pc-audio-pack",
        "version": 1,
        "sample_rate": 48000,
        "files": {},
    }
    file_manifest: dict[str, object] = manifest["files"]  # type: ignore[assignment]

    for stem, source in sources.items():
        destination = output_dir / f"{stem}.ogg"
        run_ffmpeg(ffmpeg, source, destination, stem in MUSIC_FILES)
        file_manifest[destination.name] = {
            "sha256": sha256(destination),
            "source_name": source.name,
        }
        print(f"created {destination}")

    (output_dir / "pack.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (output_dir / "README.txt").write_text(
        "Optional user-created audio pack for Tetris NES PC Port.\n"
        "This directory was generated locally and must not be committed if it "
        "contains copyrighted recordings.\n",
        encoding="utf-8",
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_dir", nargs="?", type=Path,
                        help="directory containing the named source recordings")
    parser.add_argument("output_dir", nargs="?", type=Path,
                        help="destination directory for the OGG pack")
    parser.add_argument("--check", type=Path, metavar="DIRECTORY",
                        help="validate an existing pack instead of converting")
    parser.add_argument("--ffmpeg", default="ffmpeg",
                        help="ffmpeg executable (default: ffmpeg from PATH)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.check:
        valid, problems = validate_pack(args.check)
        if valid:
            print(f"valid audio pack: {args.check}")
            return 0
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1

    if args.source_dir is None or args.output_dir is None:
        print("source_dir and output_dir are required unless --check is used", file=sys.stderr)
        return 2
    ffmpeg = shutil.which(args.ffmpeg) if not Path(args.ffmpeg).is_file() else args.ffmpeg
    if not ffmpeg:
        print("ffmpeg was not found; install it or pass --ffmpeg", file=sys.stderr)
        return 2
    try:
        build_pack(args.source_dir, args.output_dir, str(ffmpeg))
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 1
    valid, problems = validate_pack(args.output_dir)
    if not valid:
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1
    print(f"audio pack ready: {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
