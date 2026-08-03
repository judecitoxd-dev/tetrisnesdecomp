#!/usr/bin/env python3
"""Verify semantic PRG labels and rule tables against a user-supplied legal ROM.

The repository stores only addresses, hashes and small functional constants.
No PRG, CHR, audio, trace or other copyrighted payload is emitted.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib
from typing import Any


def parse_int(value: Any) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 0)


def crc32_hex(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_ines(data: bytes) -> tuple[bytes, bytes]:
    if len(data) < 16 or data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 ROM")
    trainer = 512 if data[6] & 0x04 else 0
    prg_units = data[4]
    chr_units = data[5]
    if (data[7] & 0x0C) == 0x08:
        prg_msb = data[9] & 0x0F
        chr_msb = (data[9] >> 4) & 0x0F
        if prg_msb != 0x0F:
            prg_units |= prg_msb << 8
        if chr_msb != 0x0F:
            chr_units |= chr_msb << 8
    prg_size = prg_units * 16384
    chr_size = chr_units * 8192
    start = 16 + trainer
    prg_end = start + prg_size
    chr_end = prg_end + chr_size
    if prg_end > len(data) or chr_end > len(data):
        raise ValueError("declared PRG/CHR size exceeds file length")
    return data[start:prg_end], data[prg_end:chr_end]


def cpu_offset(address: int, prg_size: int) -> int:
    if prg_size != 32768:
        raise ValueError(f"expected 32768-byte PRG, found {prg_size}")
    if not 0x8000 <= address <= 0xFFFF:
        raise ValueError(f"CPU address outside PRG window: 0x{address:04X}")
    return address - 0x8000


def bcd_word_to_int(value: int) -> int:
    result = 0
    multiplier = 1
    for shift in range(0, 16, 4):
        digit = (value >> shift) & 0x0F
        if digit > 9:
            raise ValueError(f"invalid packed BCD word 0x{value:04X}")
        result += digit * multiplier
        multiplier *= 10
    return result


def slice_at(prg: bytes, address: int, length: int) -> bytes:
    offset = cpu_offset(address, len(prg))
    if length < 1 or offset + length > len(prg):
        raise ValueError(f"range 0x{address:04X}+{length} exceeds PRG")
    return prg[offset:offset + length]


def verify_rom_bytes(data: bytes, manifest: dict[str, Any]) -> dict[str, Any]:
    errors: list[str] = []
    prg, chr_data = parse_ines(data)
    expected = manifest["rom"]

    checks = {
        "file_size": len(data),
        "crc32": crc32_hex(data),
        "sha256": sha256_hex(data),
        "prg_size": len(prg),
        "prg_crc32": crc32_hex(prg),
        "prg_sha256": sha256_hex(prg),
        "chr_size": len(chr_data),
        "chr_crc32": crc32_hex(chr_data),
        "chr_sha256": sha256_hex(chr_data),
    }
    for name, actual in checks.items():
        wanted = expected[name]
        if isinstance(actual, str):
            if actual.upper() != str(wanted).upper():
                errors.append(f"{name}: expected {wanted}, found {actual}")
        elif actual != wanted:
            errors.append(f"{name}: expected {wanted}, found {actual}")

    vector_addresses = {"nmi": 0xFFFA, "reset": 0xFFFC, "irq": 0xFFFE}
    verified_vectors = 0
    for name, vector_address in vector_addresses.items():
        raw = slice_at(prg, vector_address, 2)
        actual = raw[0] | (raw[1] << 8)
        wanted = parse_int(manifest["vectors"][name])
        if actual != wanted:
            errors.append(
                f"{name} vector: expected 0x{wanted:04X}, found 0x{actual:04X}"
            )
        else:
            verified_vectors += 1

    signature_bytes = 0
    verified_signatures = 0
    for entry in manifest.get("routine_signatures", []):
        address = parse_int(entry["address"])
        length = int(entry["length"])
        actual_hash = sha256_hex(slice_at(prg, address, length))
        if actual_hash != entry["sha256"].lower():
            errors.append(
                f"routine {entry['name']} at 0x{address:04X}: signature mismatch"
            )
        else:
            verified_signatures += 1
            signature_bytes += length

    table_bytes = 0
    verified_tables = 0
    table_by_name: dict[str, bytes] = {}
    for entry in manifest.get("tables", []):
        address = parse_int(entry["address"])
        length = int(entry["length"])
        payload = slice_at(prg, address, length)
        table_by_name[entry["name"]] = payload
        if sha256_hex(payload) != entry["sha256"].lower():
            errors.append(
                f"table {entry['name']} at 0x{address:04X}: hash mismatch"
            )
        else:
            verified_tables += 1
            table_bytes += length

    opcode_values = {"JSR": 0x20, "JMP": 0x4C}
    verified_edges = 0
    for edge in manifest.get("control_flow", []):
        source = parse_int(edge["source"])
        target = parse_int(edge["target"])
        opcode_name = str(edge["opcode"]).upper()
        raw = slice_at(prg, source, 3)
        actual_target = raw[1] | (raw[2] << 8)
        if raw[0] != opcode_values[opcode_name] or actual_target != target:
            errors.append(
                f"edge 0x{source:04X}: expected {opcode_name} 0x{target:04X}"
            )
        else:
            verified_edges += 1

    functional = manifest.get("functional_checks", {})
    functional_passed = 0

    def compare_sequence(name: str, actual: list[int], wanted: list[int]) -> None:
        nonlocal functional_passed
        if actual != wanted:
            errors.append(f"functional table {name}: values differ")
        else:
            functional_passed += 1

    compare_sequence(
        "ntsc_gravity_frames",
        list(table_by_name["ntsc_gravity_table"]),
        list(functional["ntsc_gravity_frames"]),
    )
    compare_sequence(
        "line_clear_columns",
        list(table_by_name["line_clear_columns"]),
        list(functional["line_clear_columns"]),
    )
    compare_sequence(
        "spawn_orientations",
        list(table_by_name["spawn_table"][:7]),
        list(functional["spawn_orientations"]),
    )
    compare_sequence(
        "garbage_lines",
        list(table_by_name["garbage_lines_by_clear_count"]),
        list(functional["garbage_lines"]),
    )
    point_bytes = table_by_name["score_values_bcd"]
    decoded_points = [
        bcd_word_to_int(point_bytes[index] | (point_bytes[index + 1] << 8))
        for index in range(0, len(point_bytes), 2)
    ]
    compare_sequence(
        "line_clear_points", decoded_points, list(functional["line_clear_points"])
    )

    orientation = table_by_name["orientation_table"]
    orientation_count = int(functional["orientation_count"])
    blocks_per_orientation = int(functional["blocks_per_orientation"])
    expected_size = orientation_count * blocks_per_orientation * 3
    if len(orientation) != expected_size:
        errors.append(
            f"orientation table: expected {expected_size} bytes, found {len(orientation)}"
        )
    else:
        valid_tiles = {0x7B, 0x7C, 0x7D}
        tiles = orientation[1::3]
        if any(tile not in valid_tiles for tile in tiles):
            errors.append("orientation table: unexpected mino tile identifier")
        else:
            functional_passed += 1

    return {
        "ok": not errors,
        "errors": errors,
        "rom_crc32": checks["crc32"],
        "prg_sha256": checks["prg_sha256"],
        "vectors_verified": verified_vectors,
        "routine_signatures_verified": verified_signatures,
        "routine_signature_bytes": signature_bytes,
        "tables_verified": verified_tables,
        "table_bytes": table_bytes,
        "control_flow_edges_verified": verified_edges,
        "functional_checks_verified": functional_passed,
        "note": (
            "Verified signatures and semantic checks are evidence, not binary-identical "
            "recompilation coverage."
        ),
    }


def self_test() -> int:
    prg = bytearray(32768)
    chr_data = bytes(8192)
    prg[0:6] = bytes((0x20, 0x10, 0x80, 0xEA, 0xEA, 0x60))
    prg[0x10:0x14] = bytes((0xA9, 0x01, 0x60, 0xEA))
    for address, target in ((0x7FFA, 0x8000), (0x7FFC, 0x8000), (0x7FFE, 0x8000)):
        prg[address:address + 2] = struct.pack("<H", target)
    header = bytearray(b"NES\x1a")
    header.extend((2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
    prg[0x61] = 0x7B
    rom = bytes(header) + bytes(prg) + chr_data
    manifest = {
        "rom": {
            "file_size": len(rom), "crc32": crc32_hex(rom),
            "sha256": sha256_hex(rom), "prg_size": len(prg),
            "prg_crc32": crc32_hex(prg), "prg_sha256": sha256_hex(prg),
            "chr_size": len(chr_data), "chr_crc32": crc32_hex(chr_data),
            "chr_sha256": sha256_hex(chr_data),
        },
        "vectors": {"nmi": "0x8000", "reset": "0x8000", "irq": "0x8000"},
        "routine_signatures": [{"name":"synthetic_entry","address":"0x8000","length":6,"sha256":sha256_hex(bytes(prg[:6]))}],
        "tables": [
            {"name":"ntsc_gravity_table","address":"0x8010","length":4,"sha256":sha256_hex(bytes(prg[0x10:0x14]))},
            {"name":"line_clear_columns","address":"0x8020","length":10,"sha256":sha256_hex(bytes(prg[0x20:0x2A]))},
            {"name":"spawn_table","address":"0x8030","length":8,"sha256":sha256_hex(bytes(prg[0x30:0x38]))},
            {"name":"garbage_lines_by_clear_count","address":"0x8040","length":5,"sha256":sha256_hex(bytes(prg[0x40:0x45]))},
            {"name":"score_values_bcd","address":"0x8050","length":10,"sha256":sha256_hex(bytes(prg[0x50:0x5A]))},
            {"name":"orientation_table","address":"0x8060","length":3,"sha256":sha256_hex(bytes(prg[0x60:0x63]))}
        ],
        "control_flow": [{"source":"0x8000","opcode":"JSR","target":"0x8010"}],
        "functional_checks": {
            "ntsc_gravity_frames": list(prg[0x10:0x14]),
            "line_clear_columns": [0] * 10,
            "spawn_orientations": [0] * 7,
            "garbage_lines": [0] * 5,
            "line_clear_points": [0] * 5,
            "orientation_count": 1,
            "blocks_per_orientation": 1,
        },
    }
    report = verify_rom_bytes(rom, manifest)
    if not report["ok"]:
        print("prg_verify self-test failed:", report["errors"], file=sys.stderr)
        return 1
    damaged = bytearray(rom)
    damaged[16] ^= 0x01
    if verify_rom_bytes(bytes(damaged), manifest)["ok"]:
        print("prg_verify failed to detect mutation", file=sys.stderr)
        return 1
    print("prg_verify self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path, nargs="?")
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).with_name("tetris_prg_manifest.json"))
    parser.add_argument("--report", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.rom is None:
        parser.error("ROM path is required unless --self-test is used")
    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        report = verify_rom_bytes(args.rom.read_bytes(), manifest)
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if not report["ok"]:
        for error in report["errors"]:
            print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print("PRG verification: OK "
          f"routines={report['routine_signatures_verified']} "
          f"tables={report['tables_verified']} "
          f"edges={report['control_flow_edges_verified']} "
          f"functional={report['functional_checks_verified']}")
    print("verified signature/table bytes: "
          f"{report['routine_signature_bytes'] + report['table_bytes']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
