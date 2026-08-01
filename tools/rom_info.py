#!/usr/bin/env python3
"""Inspect an iNES/NES 2.0 ROM without extracting or redistributing assets."""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import zlib


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=pathlib.Path)
    args = parser.parse_args()
    data = args.rom.read_bytes()
    if len(data) < 16 or data[:4] != b"NES\x1a":
        raise SystemExit("Not an iNES/NES 2.0 ROM")
    h = data[:16]
    nes2 = (h[7] & 0x0C) == 0x08
    mapper = (h[6] >> 4) | (h[7] & 0xF0)
    if nes2:
        mapper |= (h[8] & 0x0F) << 8
    trainer = 512 if h[6] & 4 else 0
    prg = h[4] * 16_384
    chr_size = h[5] * 8_192
    if nes2:
        prg_msb = h[9] & 0x0F
        chr_msb = h[9] >> 4
        if prg_msb != 0x0F:
            prg = ((prg_msb << 8) | h[4]) * 16_384
        if chr_msb != 0x0F:
            chr_size = ((chr_msb << 8) | h[5]) * 8_192
    expected = 16 + trainer + prg + chr_size
    print(f"File:    {args.rom}")
    print(f"Format:  {'NES 2.0' if nes2 else 'iNES'}")
    print(f"Mapper:  {mapper}")
    print(f"PRG ROM: {prg} bytes")
    print(f"CHR ROM: {chr_size} bytes")
    print(f"Size:    {len(data)} bytes (header expects at least {expected})")
    print(f"CRC32:   {zlib.crc32(data) & 0xFFFFFFFF:08X}")
    print(f"SHA-1:   {hashlib.sha1(data).hexdigest()}")
    print(f"SHA-256: {hashlib.sha256(data).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
