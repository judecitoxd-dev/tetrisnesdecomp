#!/usr/bin/env python3
"""Verify Type A ending structures in a user-provided legal Tetris NES ROM.

The tool only reports boundaries, counts and hashes. It never writes extracted
nametables, CHR banks or metasprite data.
"""

from __future__ import annotations

import argparse
import binascii
import hashlib
import json
import struct
import sys
from pathlib import Path

EXPECTED_CRC32 = 0xD16EA396
EXPECTED_FILE_SIZE = 49_168
PRG_SIZE = 32_768
CHR_SIZE = 16_384

PRG_ENDING_PALETTE = 0x2D43
PRG_TYPE_A_OVER120K_PATCH = 0x28CC
PRG_TYPE_A_ENDING = 0x5268
PRG_OAM_LOOKUP = 0x0C6C
OAM_LOOKUP_COUNT = 90

TYPE_A_BODY = (0x3E, 0x41, 0x44, 0x47, 0x4A)
TYPE_A_JETS = (0x3F, 0x40, 0x42, 0x43, 0x45,
               0x46, 0x48, 0x49, 0x23, 0x24)
TYPE_A_SPRITES = TYPE_A_BODY + TYPE_A_JETS


def parse_bulk(prg: bytes, offset: int) -> tuple[int, int]:
    position = offset
    writes = 0
    while position < len(prg):
        if prg[position] & 0x80:
            return position + 1, writes
        if position + 3 > len(prg):
            raise ValueError("truncated bulkCopyToPpu header")
        control = prg[position + 2]
        position += 3
        count = control & 0x3F
        if count == 0:
            count = 64
        position += 1 if control & 0x40 else count
        if position > len(prg):
            raise ValueError("truncated bulkCopyToPpu payload")
        writes += count
    raise ValueError("bulkCopyToPpu stream has no terminator")


def parse_patch(prg: bytes, offset: int) -> tuple[int, int]:
    if offset + 3 > len(prg):
        raise ValueError("truncated patchToPpu stream")
    position = offset + 2
    writes = 0
    while position < len(prg):
        value = prg[position]
        position += 1
        if value == 0xFD:
            return position, writes
        if value == 0xFE:
            if position + 2 > len(prg):
                raise ValueError("truncated patchToPpu address")
            position += 2
        else:
            writes += 1
    raise ValueError("patchToPpu stream has no terminator")


def metasprite(prg: bytes, sprite_index: int) -> tuple[int, bytes]:
    if not 0 <= sprite_index < OAM_LOOKUP_COUNT:
        raise ValueError("OAM index outside lookup table")
    entry = PRG_OAM_LOOKUP + sprite_index * 2
    if entry + 2 > len(prg):
        raise ValueError("truncated OAM lookup")
    address = struct.unpack_from("<H", prg, entry)[0]
    if address < 0x8000:
        raise ValueError(f"invalid OAM pointer ${address:04X}")
    offset = address - 0x8000
    position = offset
    entries = 0
    while position < len(prg) and entries < 256:
        if prg[position] == 0xFF:
            return entries, prg[offset:position + 1]
        if position + 4 > len(prg):
            raise ValueError("truncated metasprite")
        position += 4
        entries += 1
    raise ValueError("metasprite has no terminator")


def load_prg(path: Path) -> tuple[bytes, int]:
    data = path.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    if len(data) != EXPECTED_FILE_SIZE:
        raise ValueError(f"unexpected ROM size: {len(data)}")
    if data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 image")
    trainer = 512 if data[6] & 0x04 else 0
    prg_start = 16 + trainer
    prg = data[prg_start:prg_start + PRG_SIZE]
    chr_data = data[prg_start + PRG_SIZE:prg_start + PRG_SIZE + CHR_SIZE]
    if len(prg) != PRG_SIZE or len(chr_data) != CHR_SIZE:
        raise ValueError("unexpected PRG/CHR sizes")
    return prg, crc


def verify(path: Path) -> dict[str, object]:
    prg, crc = load_prg(path)
    ending_end, ending_writes = parse_bulk(prg, PRG_TYPE_A_ENDING)
    palette_end, palette_writes = parse_bulk(prg, PRG_ENDING_PALETTE)
    patch_end, patch_writes = parse_patch(prg, PRG_TYPE_A_OVER120K_PATCH)
    if ending_end != 0x56C9 or ending_writes != 1024:
        raise ValueError("Type A ending stream boundary mismatch")
    if palette_end != 0x2D67 or palette_writes != 32:
        raise ValueError("ending palette boundary mismatch")
    if patch_end != 0x2925 or patch_writes != 59:
        raise ValueError("Type A >=120k patch boundary mismatch")

    descriptors = []
    for sprite_index in TYPE_A_SPRITES:
        entry_count, raw = metasprite(prg, sprite_index)
        descriptors.append({
            "index": sprite_index,
            "entries": entry_count,
            "sha256": hashlib.sha256(raw).hexdigest(),
        })

    return {
        "rom_crc32": f"{crc:08X}",
        "tested_dump": crc == EXPECTED_CRC32,
        "ending_stream": {
            "offset": f"0x{PRG_TYPE_A_ENDING:04X}",
            "end": f"0x{ending_end:04X}",
            "writes": ending_writes,
        },
        "over_120k_patch": {
            "offset": f"0x{PRG_TYPE_A_OVER120K_PATCH:04X}",
            "end": f"0x{patch_end:04X}",
            "writes": patch_writes,
        },
        "rocket_classes": 5,
        "metasprites": descriptors,
    }


def self_test() -> None:
    fake = bytearray(0x9000)
    fake[0:5] = bytes((0x20, 0x00, 0x43, 0x7F, 0xFF))
    end, writes = parse_bulk(bytes(fake), 0)
    assert (end, writes) == (5, 3)
    fake[0:8] = bytes((0x20, 0x00, 0x11, 0x12, 0xFE, 0x20, 0x10, 0xFD))
    end, writes = parse_patch(bytes(fake), 0)
    assert (end, writes) == (8, 2)
    print("Type A ending verifier self-test passed.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.rom is None:
        parser.error("ROM path is required unless --self-test is used")
    try:
        result = verify(args.rom)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
