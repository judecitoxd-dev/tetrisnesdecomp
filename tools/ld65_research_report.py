#!/usr/bin/env python3
"""Summarize ld65 labels/debug spans without redistributing ROM bytes."""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import re
import sys
from typing import Iterable

GENERIC_LABEL = re.compile(r"^(?:\.?L[0-9A-Fa-f]{4}|__.*|\.size|\.bank)$")


def parse_record(line: str) -> tuple[str, dict[str, str]] | None:
    line = line.strip()
    if not line or "\t" not in line:
        return None
    kind, payload = line.split("\t", 1)
    fields: dict[str, str] = {}
    for item in next(csv.reader([payload], skipinitialspace=True)):
        if "=" not in item:
            continue
        key, value = item.split("=", 1)
        fields[key.strip()] = value.strip().strip('"')
    return kind, fields


def number(value: str | None, default: int = 0) -> int:
    if value is None:
        return default
    return int(value, 0)


def interval_union_size(intervals: Iterable[tuple[int, int]]) -> int:
    ranges = sorted((start, end) for start, end in intervals if end > start)
    if not ranges:
        return 0
    total = 0
    current_start, current_end = ranges[0]
    for start, end in ranges[1:]:
        if start <= current_end:
            current_end = max(current_end, end)
        else:
            total += current_end - current_start
            current_start, current_end = start, end
    return total + current_end - current_start


def parse_labels(path: Path) -> dict[str, object]:
    names: list[str] = []
    addresses: list[int] = []
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = raw.split()
        if len(parts) < 3:
            continue
        try:
            address = int(parts[1], 16)
        except ValueError:
            continue
        name = parts[2].lstrip(".")
        names.append(name)
        addresses.append(address)
    unique_names = sorted(set(names))
    semantic = [name for name in unique_names if not GENERIC_LABEL.match(name)]
    generic = [name for name in unique_names if GENERIC_LABEL.match(name)]
    prg_addresses = [address for address in addresses if 0x8000 <= address <= 0xFFFF]
    return {
        "labels_total": len(unique_names),
        "labels_semantic": len(semantic),
        "labels_generic": len(generic),
        "labels_in_prg_window": len(set(prg_addresses)),
        "semantic_label_ratio_percent": round(
            len(semantic) * 100.0 / len(unique_names), 4
        ) if unique_names else 0.0,
    }


def parse_debug(path: Path) -> dict[str, object]:
    segments: dict[int, dict[str, object]] = {}
    spans_by_segment: dict[int, list[tuple[int, int]]] = {}
    source_files: set[str] = set()
    line_records = 0
    symbol_records = 0

    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parsed = parse_record(raw)
        if parsed is None:
            continue
        kind, fields = parsed
        if kind == "seg":
            segment_id = number(fields.get("id"), -1)
            if segment_id >= 0:
                segments[segment_id] = {
                    "id": segment_id,
                    "name": fields.get("name", ""),
                    "start": number(fields.get("start")),
                    "size": number(fields.get("size")),
                    "output": fields.get("oname", ""),
                    "output_offset": number(fields.get("ooffs")),
                }
        elif kind == "span":
            segment_id = number(fields.get("seg"), -1)
            start = number(fields.get("start"))
            size = number(fields.get("size"))
            if segment_id >= 0 and size > 0:
                spans_by_segment.setdefault(segment_id, []).append((start, start + size))
        elif kind == "file":
            if fields.get("name"):
                source_files.add(fields["name"])
        elif kind == "line":
            line_records += 1
        elif kind in {"sym", "csym"}:
            symbol_records += 1

    prg_segment_ids: list[int] = []
    prg_segment_bytes = 0
    mapped_prg_bytes = 0
    segment_report: list[dict[str, object]] = []
    for segment_id, segment in sorted(segments.items()):
        name = str(segment["name"])
        start = int(segment["start"])
        size = int(segment["size"])
        is_prg = (
            name.startswith("PRG")
            or name.startswith("unreferenced_data")
            or name == "VECTORS"
            or (0x8000 <= start <= 0xFFFF and size > 0)
        )
        mapped = interval_union_size(spans_by_segment.get(segment_id, []))
        segment_report.append({
            **segment,
            "mapped_source_bytes": mapped,
            "source_mapping_percent": round(mapped * 100.0 / size, 4) if size else 0.0,
            "is_prg": is_prg,
        })
        if is_prg:
            prg_segment_ids.append(segment_id)
            prg_segment_bytes += size
            mapped_prg_bytes += min(mapped, size)

    return {
        "segments": segment_report,
        "prg_segment_ids": prg_segment_ids,
        "prg_segment_bytes": prg_segment_bytes,
        "mapped_prg_source_bytes": mapped_prg_bytes,
        "prg_layout_coverage_percent": round(prg_segment_bytes * 100.0 / 32768, 4),
        "prg_source_mapping_percent": round(mapped_prg_bytes * 100.0 / 32768, 4),
        "source_files": len(source_files),
        "line_records": line_records,
        "symbol_records": symbol_records,
    }


def build_report(labels: Path, debug: Path, revision: str) -> dict[str, object]:
    label_report = parse_labels(labels)
    debug_report = parse_debug(debug)
    exact_layout = debug_report["prg_segment_bytes"] == 32768
    complete_mapping = debug_report["mapped_prg_source_bytes"] == 32768
    return {
        "schema": 1,
        "source_revision": revision,
        "labels": label_report,
        "debug": debug_report,
        "gates": {
            "prg_layout_is_32768_bytes": exact_layout,
            "all_prg_bytes_have_source_spans": complete_mapping,
        },
        "source_partition_percent": 100 if exact_layout and complete_mapping else round(
            min(debug_report["prg_segment_bytes"], debug_report["mapped_prg_source_bytes"])
            * 100.0 / 32768,
            4,
        ),
        "interpretation": (
            "Source partition coverage proves that ld65 maps every PRG byte to source. "
            "Semantic understanding is measured separately by names and subsystem audits."
        ),
    }


def self_test() -> int:
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        labels = root / "sample.lbl"
        debug = root / "sample.dbg"
        labels.write_text("al 008000 .reset\nal 008010 .L8010\n", encoding="utf-8")
        debug.write_text(
            "seg\tid=0,name=\"PRG_chunk1\",start=0x8000,size=0x8000,oname=\"x.nes\",ooffs=16\n"
            "span\tid=0,seg=0,start=0,size=0x4000\n"
            "span\tid=1,seg=0,start=0x4000,size=0x4000\n"
            "file\tid=0,name=\"main.asm\"\n"
            "line\tid=0,file=0,line=1,span=0\n",
            encoding="utf-8",
        )
        report = build_report(labels, debug, "self-test")
        if report["source_partition_percent"] != 100:
            print("ld65_research_report self-test failed", file=sys.stderr)
            return 1
    print("ld65_research_report self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("labels", type=Path, nargs="?")
    parser.add_argument("debug", type=Path, nargs="?")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--source-revision", default="unknown")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.labels is None or args.debug is None:
        parser.error("labels and debug files are required unless --self-test is used")
    try:
        report = build_report(args.labels, args.debug, args.source_revision)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "ld65 research report: "
        f"partition={report['source_partition_percent']}% "
        f"semantic_labels={report['labels']['labels_semantic']} "
        f"generic_labels={report['labels']['labels_generic']}"
    )
    return 0 if report["gates"]["prg_layout_is_32768_bytes"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
