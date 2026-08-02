#!/usr/bin/env python3
"""Verify B-Type cathedral tables and metasprites in a legal ROM.

Only offsets, lengths and cryptographic hashes are stored here. The tool never
contains or writes the original table/metasprite bytes.
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
EXPECTED_SIZE = 49_168
PRG_SIZE = 32_768
CHR_SIZE = 16_384
PRG_OAM_LOOKUP = 0x0C6C
OAM_LOOKUP_COUNT = 90

TABLES = {
    "animation_speed": (0x2749, 10,
        "8bdc09788ba60d7ecdc9d037f7adaab182b9a02c0fea47fb780ef853d7f1b58f"),
    "frame_delay": (0x2753, 10,
        "bd354850f807d8a032d2063a6ec4caef79b70504e105f4f894d35751e2f29bb2"),
    "start_x": (0x275D, 10,
        "c8fa5249387c9ed9680f5ff753718e2271a3f25282b7e025d7f55871286acdbe"),
    "sentinel_x": (0x2767, 10,
        "d49b00015b09b764170be5d0149d4b5a9bb804034be1bc0408172c4a36266a53"),
    "vector_x": (0x2771, 10,
        "9257cc984dfe89d2012b0630074302f300db9ab4951cc42fb7f75d58ea2e9bea"),
    "trigger_x": (0x277B, 60,
        "9ccad52616b5ff911c144255ba2b58945e95123c5245d837acfb1acc38810b08"),
    "position_y": (0x27B7, 60,
        "616a8f595a6ce420714c4f7b87aa7832c66683e813ba7e49a0f267d0e4fa9ce7"),
    "sprite_base": (0x27F3, 10,
        "5e07decbaf0fa3ce5e8881ffc704ae51a48f2b571d50293a48d3ab86f8d5308f"),
}

CATHEDRAL_SPRITES = (
    0x2C,0x2D,0x2E,0x2F,0x54,0x55,
    0x32,0x33,0x34,0x35,0x36,0x37,
    0x4B,0x4C,0x38,0x39,0x3A,0x3B,
)


def load_prg(path: Path) -> tuple[bytes, int]:
    data = path.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    if len(data) != EXPECTED_SIZE:
        raise ValueError(f"unexpected ROM size: {len(data)}")
    if data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 image")
    trainer = 512 if data[6] & 0x04 else 0
    start = 16 + trainer
    prg = data[start:start + PRG_SIZE]
    chr_data = data[start + PRG_SIZE:start + PRG_SIZE + CHR_SIZE]
    if len(prg) != PRG_SIZE or len(chr_data) != CHR_SIZE:
        raise ValueError("unexpected PRG/CHR sizes")
    return prg, crc


def metasprite(prg: bytes, index: int) -> tuple[int, bytes]:
    if not 0 <= index < OAM_LOOKUP_COUNT:
        raise ValueError("OAM index outside lookup table")
    entry = PRG_OAM_LOOKUP + index * 2
    address = struct.unpack_from("<H", prg, entry)[0]
    if address < 0x8000:
        raise ValueError(f"invalid OAM pointer for {index:02X}: ${address:04X}")
    offset = address - 0x8000
    position = offset
    count = 0
    while position < len(prg) and count < 64:
        if prg[position] == 0xFF:
            return count, prg[offset:position + 1]
        if position + 4 > len(prg):
            raise ValueError(f"truncated metasprite {index:02X}")
        position += 4
        count += 1
    raise ValueError(f"metasprite {index:02X} has no terminator")


def verify(path: Path) -> dict[str, object]:
    prg, crc = load_prg(path)
    table_results: dict[str, object] = {}
    for name, (offset, length, expected_hash) in TABLES.items():
        raw = prg[offset:offset + length]
        actual_hash = hashlib.sha256(raw).hexdigest()
        if len(raw) != length or actual_hash != expected_hash:
            raise ValueError(f"{name} mismatch at PRG+0x{offset:04X}")
        table_results[name] = {
            "offset": f"0x{offset:04X}",
            "bytes": length,
            "sha256": actual_hash,
        }

    sprites = []
    for index in CATHEDRAL_SPRITES:
        entries, raw = metasprite(prg, index)
        sprites.append({
            "index": index,
            "entries": entries,
            "sha256": hashlib.sha256(raw).hexdigest(),
        })

    return {
        "rom_crc32": f"{crc:08X}",
        "tested_dump": crc == EXPECTED_CRC32,
        "tables": table_results,
        "metasprite_count": len(sprites),
        "metasprites": sprites,
    }


def self_test() -> None:
    assert len(TABLES) == 8
    assert sum(length for _, length, _ in TABLES.values()) == 180
    assert all(len(digest) == 64 for _, _, digest in TABLES.values())
    assert len(CATHEDRAL_SPRITES) == 18
    assert len(set(CATHEDRAL_SPRITES)) == 18
    print("Cathedral verifier self-test passed.")


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
