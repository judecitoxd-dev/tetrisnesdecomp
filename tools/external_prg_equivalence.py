#!/usr/bin/env python3
"""Compare an externally rebuilt NES image with the legal target PRG metadata.

The tool stores hashes and coverage facts only. It never copies ROM/PRG/CHR bytes
into the repository or the generated report.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import zlib


def parse_ines(data: bytes) -> tuple[bytes, bytes]:
    if len(data) < 16 or data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 image")
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
    start = 16 + trainer
    prg_size = prg_units * 16384
    chr_size = chr_units * 8192
    end = start + prg_size + chr_size
    if end > len(data):
        raise ValueError("declared PRG/CHR payload exceeds file length")
    return data[start:start + prg_size], data[start + prg_size:end]


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def crc32_hex(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"


def describe(data: bytes, prg: bytes, chr_data: bytes) -> dict[str, object]:
    return {
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


def verify(candidate: Path, manifest: Path, source_revision: str) -> dict[str, object]:
    expected_manifest = json.loads(manifest.read_text(encoding="utf-8"))
    expected = expected_manifest["rom"]
    data = candidate.read_bytes()
    prg, chr_data = parse_ines(data)
    actual = describe(data, prg, chr_data)
    target_keys = (
        "file_size", "crc32", "sha256", "prg_size", "prg_crc32",
        "prg_sha256", "chr_size", "chr_crc32", "chr_sha256",
    )
    target = {key: expected[key] for key in target_keys}
    full_match = str(actual["sha256"]).lower() == str(target["sha256"]).lower()
    prg_match = str(actual["prg_sha256"]).lower() == str(target["prg_sha256"]).lower()
    chr_match = str(actual["chr_sha256"]).lower() == str(target["chr_sha256"]).lower()
    return {
        "schema": 1,
        "source_revision": source_revision,
        "candidate": actual,
        "target": target,
        "full_rom_match": full_match,
        "prg_match": prg_match,
        "chr_match": chr_match,
        "prg_length_match": actual["prg_size"] == target["prg_size"],
        "binary_prg_reconstruction_percent": 100 if prg_match else 0,
        "interpretation": (
            "100 means the rebuilt 32768-byte PRG is byte-identical by SHA-256. "
            "It does not by itself mean every symbol has a semantic name."
        ),
    }


def self_test() -> int:
    header = bytearray(b"NES\x1a")
    header.extend((2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
    prg = bytes((index * 17 + 3) & 0xFF for index in range(32768))
    chr_data = bytes((index * 7 + 1) & 0xFF for index in range(8192))
    image = bytes(header) + prg + chr_data
    parsed_prg, parsed_chr = parse_ines(image)
    if parsed_prg != prg or parsed_chr != chr_data:
        print("external_prg_equivalence self-test failed", file=sys.stderr)
        return 1
    if sha256_hex(parsed_prg) != hashlib.sha256(prg).hexdigest():
        print("external_prg_equivalence hash self-test failed", file=sys.stderr)
        return 1
    print("external_prg_equivalence self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("candidate", type=Path, nargs="?")
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).with_name("tetris_prg_manifest.json"))
    parser.add_argument("--report", type=Path)
    parser.add_argument("--source-revision", default="unknown")
    parser.add_argument("--allow-prg-mismatch", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.candidate is None:
        parser.error("candidate NES image is required unless --self-test is used")
    try:
        report = verify(args.candidate, args.manifest, args.source_revision)
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "external PRG equivalence: "
        f"prg_match={report['prg_match']} "
        f"full_rom_match={report['full_rom_match']} "
        f"chr_match={report['chr_match']}"
    )
    if not report["prg_match"] and not args.allow_prg_mismatch:
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
