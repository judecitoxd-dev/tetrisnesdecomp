#!/usr/bin/env python3
"""Verify PPU, OAM and demo structures in the user's legal Tetris ROM.

No extracted asset is written. The output contains only offsets, counts and
checksums useful for regression and decompilation work.
"""

from __future__ import annotations

import argparse
import binascii
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

EXPECTED_CRC32 = 0xD16EA396
EXPECTED_FILE_SIZE = 49_168
EXPECTED_PRG_SIZE = 32_768
EXPECTED_CHR_SIZE = 16_384
MAX_OAM_DESCRIPTOR_ENTRIES = 512

BULK_STREAMS = (
    ("ending_palette", 0x2D43, 0x2D67, 32),
    ("game_type_menu", 0x367A, 0x3ADB, 1024),
    ("level_menu", 0x3ADB, 0x3F3C, 1024),
    ("game_screen", 0x3F3C, 0x439D, 1024),
    ("enter_high_score", 0x439D, 0x47FE, 1024),
    ("high_score_patch", 0x47FE, 0x495D, 320),
    ("height_menu_patch", 0x495D, 0x49A6, 120),
    ("b_type_castle", 0x49A6, 0x4E07, 1024),
    ("b_type_normal", 0x4E07, 0x5268, 1024),
)

PATCH_STREAMS = (
    ("concert_height_0", 0x2834, 0x284A, 10, 4),
    ("concert_height_1", 0x284A, 0x2862, 12, 4),
    ("concert_height_2", 0x2862, 0x287A, 12, 4),
    ("concert_height_3", 0x287A, 0x2896, 16, 4),
    ("concert_height_4", 0x2896, 0x28A8, 9, 3),
)

OAM_LOOKUP_OFFSET = 0x0C6C
OAM_LOOKUP_COUNT = 90
DEMO_BUTTONS_OFFSET = 0x5D00
DEMO_BUTTONS_SIZE = 0x0200
DEMO_PIECES_OFFSET = 0x5F00
DEMO_PIECES_SIZE = 0x0100


class VerificationError(RuntimeError):
    pass


@dataclass(frozen=True)
class BulkResult:
    end: int
    writes: int
    records: int


@dataclass(frozen=True)
class PatchResult:
    end: int
    writes: int
    segments: int


def require(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError(message)


def parse_bulk_stream(data: bytes, offset: int) -> BulkResult:
    position = offset
    writes = 0
    records = 0
    require(0 <= position < len(data), f"bulk offset 0x{offset:X} outside PRG")
    while position < len(data):
        if data[position] & 0x80:
            return BulkResult(position + 1, writes, records)
        require(position + 3 <= len(data), "truncated bulk header")
        address = (data[position] << 8) | data[position + 1]
        control = data[position + 2]
        position += 3
        count = control & 0x3F or 64
        payload = 1 if control & 0x40 else count
        require(0x2000 <= address <= 0x3FFF,
                f"unexpected bulk PPU address 0x{address:04X}")
        require(position + payload <= len(data), "truncated bulk payload")
        position += payload
        writes += count
        records += 1
    raise VerificationError("bulk stream has no terminator")


def parse_patch_stream(data: bytes, offset: int) -> PatchResult:
    position = offset
    writes = 0
    segments = 1
    require(position + 2 <= len(data), "truncated patch address")
    address = (data[position] << 8) | data[position + 1]
    position += 2
    require(0x2000 <= address < 0x2400,
            f"unexpected patch address 0x{address:04X}")
    while position < len(data):
        value = data[position]
        position += 1
        if value == 0xFD:
            return PatchResult(position, writes, segments)
        if value == 0xFE:
            require(position + 2 <= len(data), "truncated patch segment")
            address = (data[position] << 8) | data[position + 1]
            position += 2
            segments += 1
            require(0x2000 <= address < 0x2400,
                    f"unexpected patch address 0x{address:04X}")
            continue
        require(0x2000 <= address < 0x2400,
                f"patch write outside nametable at 0x{address:04X}")
        address += 1
        writes += 1
    raise VerificationError("patch stream has no FD terminator")


def parse_oam_descriptor(prg: bytes, offset: int) -> int:
    position = offset
    entries = 0
    require(0 <= position < len(prg), f"OAM offset 0x{offset:X} outside PRG")
    while position < len(prg) and entries < MAX_OAM_DESCRIPTOR_ENTRIES:
        if prg[position] == 0xFF:
            return entries
        require(position + 4 <= len(prg), "truncated OAM entry")
        position += 4
        entries += 1
    raise VerificationError(
        f"OAM descriptor at 0x{offset:X} has no terminator within "
        f"{MAX_OAM_DESCRIPTOR_ENTRIES} entries"
    )


def verify_oam_lookup(prg: bytes) -> dict[str, int]:
    require(OAM_LOOKUP_OFFSET + OAM_LOOKUP_COUNT * 2 <= len(prg),
            "OAM pointer table outside PRG")
    total = 0
    largest = 0
    unique_offsets: set[int] = set()
    for index in range(OAM_LOOKUP_COUNT):
        entry = OAM_LOOKUP_OFFSET + index * 2
        address = struct.unpack_from("<H", prg, entry)[0]
        require(0x8000 <= address <= 0xFFFF,
                f"invalid OAM pointer {index}: 0x{address:04X}")
        offset = address - 0x8000
        count = parse_oam_descriptor(prg, offset)
        unique_offsets.add(offset)
        total += count
        largest = max(largest, count)
    return {
        "pointer_count": OAM_LOOKUP_COUNT,
        "unique_descriptors": len(unique_offsets),
        "descriptor_entries_including_aliases": total,
        "largest_descriptor": largest,
    }


def read_rom(path: Path) -> tuple[bytes, bytes, dict[str, int | str]]:
    raw = path.read_bytes()
    require(len(raw) == EXPECTED_FILE_SIZE,
            f"file size {len(raw)}, expected {EXPECTED_FILE_SIZE}")
    require(raw[:4] == b"NES\x1A", "invalid iNES header")
    prg_size = raw[4] * 16_384
    chr_size = raw[5] * 8_192
    mapper = (raw[6] >> 4) | (raw[7] & 0xF0)
    require(prg_size == EXPECTED_PRG_SIZE, f"unexpected PRG size {prg_size}")
    require(chr_size == EXPECTED_CHR_SIZE, f"unexpected CHR size {chr_size}")
    require(mapper == 1, f"unexpected mapper {mapper}")
    payload = 16 + (512 if raw[6] & 0x04 else 0)
    prg = raw[payload:payload + prg_size]
    chr_data = raw[payload + prg_size:payload + prg_size + chr_size]
    require(len(prg) == prg_size and len(chr_data) == chr_size,
            "truncated ROM payload")
    crc32 = binascii.crc32(raw) & 0xFFFFFFFF
    require(crc32 == EXPECTED_CRC32,
            f"CRC32 {crc32:08X}, expected {EXPECTED_CRC32:08X}")
    return prg, chr_data, {
        "crc32": f"{crc32:08X}",
        "file_size": len(raw),
        "prg_size": len(prg),
        "chr_size": len(chr_data),
        "mapper": mapper,
    }


def verify_rom(path: Path) -> dict[str, object]:
    prg, chr_data, result = read_rom(path)
    bulk: dict[str, dict[str, int | str]] = {}
    for name, start, expected_end, expected_writes in BULK_STREAMS:
        parsed = parse_bulk_stream(prg, start)
        require(parsed.end == expected_end,
                f"{name} ends 0x{parsed.end:X}, expected 0x{expected_end:X}")
        require(parsed.writes == expected_writes,
                f"{name} writes {parsed.writes}, expected {expected_writes}")
        bulk[name] = {
            "start": f"0x{start:04X}", "end": f"0x{parsed.end:04X}",
            "writes": parsed.writes, "records": parsed.records,
        }

    patches: dict[str, dict[str, int | str]] = {}
    for name, start, expected_end, expected_writes, expected_segments in PATCH_STREAMS:
        parsed = parse_patch_stream(prg, start)
        require(parsed.end == expected_end,
                f"{name} ends 0x{parsed.end:X}, expected 0x{expected_end:X}")
        require(parsed.writes == expected_writes,
                f"{name} writes {parsed.writes}, expected {expected_writes}")
        require(parsed.segments == expected_segments,
                f"{name} has {parsed.segments} segments, expected {expected_segments}")
        patches[name] = {
            "start": f"0x{start:04X}", "end": f"0x{parsed.end:04X}",
            "writes": parsed.writes, "segments": parsed.segments,
        }

    require(DEMO_BUTTONS_OFFSET + DEMO_BUTTONS_SIZE == DEMO_PIECES_OFFSET,
            "demo tables are not contiguous")
    require(DEMO_PIECES_OFFSET + DEMO_PIECES_SIZE <= len(prg),
            "demo tables outside PRG")
    nonzero_pairs = sum(
        prg[index] != 0 or prg[index + 1] != 0
        for index in range(DEMO_BUTTONS_OFFSET,
                           DEMO_BUTTONS_OFFSET + DEMO_BUTTONS_SIZE, 2)
    )

    result.update({
        "chr_crc32": f"{binascii.crc32(chr_data) & 0xFFFFFFFF:08X}",
        "bulk_streams": bulk,
        "patch_streams": patches,
        "oam": verify_oam_lookup(prg),
        "demo": {
            "buttons_offset": f"0x{DEMO_BUTTONS_OFFSET:04X}",
            "buttons_size": DEMO_BUTTONS_SIZE,
            "nonzero_command_pairs": nonzero_pairs,
            "pieces_offset": f"0x{DEMO_PIECES_OFFSET:04X}",
            "pieces_size": DEMO_PIECES_SIZE,
        },
    })
    return result


def self_test() -> None:
    bulk = bytes((0x20, 0x00, 0x43, 0xAA,
                  0x20, 0x10, 0x02, 0x01, 0x02, 0xFF))
    require(parse_bulk_stream(bulk, 0) == BulkResult(10, 5, 2),
            "bulk self-test failed")
    patch = bytes((0x20, 0x00, 0x11, 0x22,
                   0xFE, 0x20, 0x10, 0x33, 0xFD))
    require(parse_patch_stream(patch, 0) == PatchResult(9, 3, 2),
            "patch self-test failed")
    descriptor = bytes((0, 1, 2, 3, 4, 5, 6, 7, 0xFF))
    require(parse_oam_descriptor(descriptor, 0) == 2,
            "OAM self-test failed")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", nargs="?", type=Path)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
            print("ROM asset parser self-test passed.")
            return 0
        if args.rom is None:
            parser.error("ROM path required unless --self-test is used")
        result = verify_rom(args.rom)
        if args.json:
            print(json.dumps(result, indent=2, sort_keys=True))
        else:
            oam = result["oam"]
            demo = result["demo"]
            print(f"Verified ROM CRC32 {result['crc32']}")
            print(f"Bulk streams: {len(result['bulk_streams'])}")
            print(f"Direct patches: {len(result['patch_streams'])}")
            print(f"OAM pointers: {oam['pointer_count']}; "
                  f"largest descriptor: {oam['largest_descriptor']} entries")
            print(f"Demo: {demo['buttons_size']} button bytes, "
                  f"{demo['pieces_size']} piece bytes")
        return 0
    except (OSError, VerificationError) as exc:
        print(f"verification failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
