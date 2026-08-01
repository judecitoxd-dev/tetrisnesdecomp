#!/usr/bin/env python3
"""Locate small gameplay tables in a user-supplied Tetris (USA) NES ROM.

This does not extract or redistribute the game. It reports offsets, CPU addresses,
header metadata and interrupt vectors so changes to the native port can be checked
against a legally obtained cartridge dump.
"""

from __future__ import annotations

import argparse
import binascii
import hashlib
from pathlib import Path
import sys

TABLES: dict[str, bytes] = {
    "ntsc_gravity": bytes([
        0x30, 0x2B, 0x26, 0x21, 0x1C, 0x17, 0x12, 0x0D,
        0x08, 0x06, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04,
        0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x01,
    ]),
    "line_clear_columns": bytes([4, 3, 2, 1, 0, 5, 6, 7, 8, 9]),
    "orientation_lookup": bytes([
        0x02, 0x07, 0x08, 0x0A, 0x0B, 0x0E, 0x12,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x07, 0x07,
        0x07, 0x07, 0x08, 0x08, 0x0A, 0x0B, 0x0B,
        0x0E, 0x0E, 0x0E, 0x0E, 0x12, 0x12,
    ]),
    "score_values_bcd": bytes([0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x12]),
    "level_palettes": bytes.fromhex(
        "0F3021120F30291A0F3024140F302A12"
        "0F302B150F30222B0F3000160F300513"
        "0F3016120F302716"
    ),
}


def parse_rom(path: Path) -> tuple[bytes, bytes, int, bool, int]:
    data = path.read_bytes()
    if len(data) < 16 or data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 ROM")
    nes2 = (data[7] & 0x0C) == 0x08
    mapper = (data[6] >> 4) | (data[7] & 0xF0)
    if nes2:
        mapper |= (data[8] & 0x0F) << 8
    trainer = 512 if data[6] & 0x04 else 0
    prg_size = data[4] * 16384
    chr_size = data[5] * 8192
    if nes2:
        prg_msb = data[9] & 0x0F
        chr_msb = data[9] >> 4
        if prg_msb != 0x0F:
            prg_size = ((prg_msb << 8) | data[4]) * 16384
        if chr_msb != 0x0F:
            chr_size = ((chr_msb << 8) | data[5]) * 8192
    start = 16 + trainer
    end = start + prg_size
    if end + chr_size > len(data):
        raise ValueError("header sizes exceed file length")
    return data, data[start:end], mapper, nes2, chr_size


def vector(prg: bytes, offset_from_end: int) -> int:
    start = len(prg) + offset_from_end if offset_from_end < 0 else offset_from_end
    return int.from_bytes(prg[start:start + 2], "little")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    args = parser.parse_args()
    try:
        data, prg, mapper, nes2, chr_size = parse_rom(args.rom)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"file: {args.rom}")
    print(f"format: {'NES 2.0' if nes2 else 'iNES'}")
    print(f"mapper: {mapper}")
    print(f"prg_size: {len(prg)}")
    print(f"chr_size: {chr_size}")
    print(f"crc32: {binascii.crc32(data) & 0xFFFFFFFF:08X}")
    print(f"sha1: {hashlib.sha1(data).hexdigest()}")
    print(f"nmi_vector: ${vector(prg, -6):04X}")
    print(f"reset_vector: ${vector(prg, -4):04X}")
    print(f"irq_vector: ${vector(prg, -2):04X}")
    print("tables:")
    missing = False
    for name, pattern in TABLES.items():
        offset = prg.find(pattern)
        if offset < 0:
            print(f"  {name}: not found")
            missing = True
        else:
            print(f"  {name}: prg+0x{offset:04X} cpu=${0x8000 + offset:04X} bytes={len(pattern)}")
    return 2 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
