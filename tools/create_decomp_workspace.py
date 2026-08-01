#!/usr/bin/env python3
"""Create a private reverse-engineering workspace from a user-supplied ROM.

Generated bank binaries and assembly listings remain outside the repository. This
script is intended for legal personal dumps and never downloads or embeds a ROM.
"""

from __future__ import annotations

import argparse
import binascii
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def parse_rom(path: Path) -> tuple[bytes, bytes, bytes]:
    data = path.read_bytes()
    if len(data) < 16 or data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 ROM")
    trainer = 512 if data[6] & 0x04 else 0
    prg_size = data[4] * 16384
    chr_size = data[5] * 8192
    if (data[7] & 0x0C) == 0x08:
        prg_msb = data[9] & 0x0F
        chr_msb = data[9] >> 4
        if prg_msb != 0x0F:
            prg_size = ((prg_msb << 8) | data[4]) * 16384
        if chr_msb != 0x0F:
            chr_size = ((chr_msb << 8) | data[5]) * 8192
    start = 16 + trainer
    prg_end = start + prg_size
    chr_end = prg_end + chr_size
    if chr_end > len(data):
        raise ValueError("header sizes exceed file length")
    if prg_size != 32768:
        raise ValueError(f"expected 32768-byte PRG, found {prg_size}")
    return data, data[start:prg_end], data[prg_end:chr_end]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command: list[str]) -> None:
    print("+", " ".join(command))
    subprocess.run(command, check=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()) and not args.force:
        print("error: output directory is not empty; use --force", file=sys.stderr)
        return 1
    output.mkdir(parents=True, exist_ok=True)

    try:
        data, prg, chr_data = parse_rom(args.rom)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    (output / "bank0_8000_bfff.bin").write_bytes(prg[:0x4000])
    (output / "bank1_c000_ffff.bin").write_bytes(prg[0x4000:])
    (output / "chr.bin").write_bytes(chr_data)

    tool_dir = Path(__file__).resolve().parent
    python = sys.executable
    run([python, str(tool_dir / "disassemble_prg.py"), str(args.rom),
         "-o", str(output / "prg_conservative.asm"),
         "--report", str(output / "conservative_report.json"),
         "--dot", str(output / "conservative_callgraph.dot")])
    run([python, str(tool_dir / "disassemble_prg.py"), str(args.rom),
         "--aggressive", "-o", str(output / "prg_aggressive.asm"),
         "--report", str(output / "aggressive_report.json"),
         "--dot", str(output / "aggressive_callgraph.dot")])
    run([python, str(tool_dir / "rom_info.py"), str(args.rom)])
    run([python, str(tool_dir / "rom_tables.py"), str(args.rom)])

    files = sorted(path for path in output.iterdir() if path.is_file())
    manifest = {
        "source_rom": str(args.rom.resolve()),
        "rom_crc32": f"{binascii.crc32(data) & 0xFFFFFFFF:08X}",
        "rom_sha1": hashlib.sha1(data).hexdigest(),
        "rom_sha256": hashlib.sha256(data).hexdigest(),
        "prg_size": len(prg),
        "chr_size": len(chr_data),
        "files": {path.name: {"size": path.stat().st_size, "sha256": digest(path)}
                  for path in files},
        "notice": "Generated locally from a user-supplied ROM. Do not commit or redistribute extracted binaries."
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n",
                                           encoding="utf-8")
    (output / "README.txt").write_text(
        "Private Tetris NES decompilation workspace\n\n"
        "This directory was generated from your own ROM. The .bin and .asm files may contain\n"
        "copyrighted game data and must not be committed to the source repository or shared.\n"
        "Use the JSON reports to distinguish conservative recursive coverage from aggressive\n"
        "heuristic coverage. Neither percentage equals verified decompilation progress.\n",
        encoding="utf-8")
    print(f"workspace: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
