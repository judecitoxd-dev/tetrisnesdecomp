#!/usr/bin/env python3
"""Compare frame-indexed CSV traces from the port and an emulator."""

from __future__ import annotations

import argparse
import csv
import json
import sys
import tempfile
from pathlib import Path
from typing import Any, Iterable, Sequence

EXIT_MATCH = 0
EXIT_DIFFERENT = 1
EXIT_USAGE = 2
DEFAULT_IGNORED = {"comment", "notes"}


class TraceError(ValueError):
    pass


def parse_list(value: str | None) -> list[str] | None:
    if value is None:
        return None
    items = [item.strip() for item in value.split(",") if item.strip()]
    if not items:
        raise TraceError("column list is empty")
    return items


def parse_number(text: str, column: str) -> int | None:
    value = text.strip().lower().replace("_", "")
    if not value:
        return None
    aliases = {"true": 1, "yes": 1, "on": 1, "false": 0, "no": 0, "off": 0}
    if value in aliases:
        return aliases[value]
    try:
        if value.startswith("$"):
            return int(value[1:], 16)
        if value.startswith("0x"):
            return int(value[2:], 16)
        if value.startswith("%"):
            return int(value[1:], 2)
        if value.startswith("0b"):
            return int(value[2:], 2)
        if value.startswith("-") or value.isdigit():
            return int(value, 10)
        if "hash" in column.lower() and all(ch in "0123456789abcdef" for ch in value):
            return int(value, 16)
    except ValueError:
        return None
    return None


def normalize(value: str | None, column: str) -> Any:
    if value is None:
        return None
    number = parse_number(value, column)
    return number if number is not None else value.strip().casefold()


def load_trace(path: Path, frame_column: str) -> tuple[list[str], dict[int, dict[str, str]]]:
    try:
        handle = path.open("r", encoding="utf-8-sig", newline="")
    except OSError as exc:
        raise TraceError(f"cannot open {path}: {exc}") from exc

    with handle:
        reader = csv.DictReader(
            line for line in handle if line.strip() and not line.lstrip().startswith("#")
        )
        if not reader.fieldnames:
            raise TraceError(f"{path}: missing CSV header")
        fields = [field.strip() for field in reader.fieldnames if field]
        if frame_column not in fields:
            raise TraceError(f"{path}: missing frame column {frame_column!r}")

        rows: dict[int, dict[str, str]] = {}
        for line_number, raw in enumerate(reader, start=2):
            frame_text = raw.get(frame_column, "")
            frame = parse_number(frame_text or "", frame_column)
            if frame is None or frame < 0:
                raise TraceError(
                    f"{path}:{line_number}: invalid {frame_column} {frame_text!r}"
                )
            if frame in rows:
                raise TraceError(f"{path}:{line_number}: duplicate frame {frame}")
            rows[frame] = {
                key.strip(): (value or "").strip()
                for key, value in raw.items()
                if key is not None
            }

    if not rows:
        raise TraceError(f"{path}: trace contains no data rows")
    return fields, rows


def comparable_columns(
    reference_fields: Sequence[str],
    candidate_fields: Sequence[str],
    requested: Sequence[str] | None,
    ignored: Iterable[str],
    frame_column: str,
    allow_missing: bool,
) -> list[str]:
    ignored_set = set(ignored) | {frame_column}
    if requested is None:
        columns = [
            field
            for field in reference_fields
            if field in candidate_fields and field not in ignored_set
        ]
    else:
        columns = [field for field in requested if field not in ignored_set]

    missing_ref = [field for field in columns if field not in reference_fields]
    missing_candidate = [field for field in columns if field not in candidate_fields]
    if (missing_ref or missing_candidate) and not allow_missing:
        details = []
        if missing_ref:
            details.append("reference missing " + ", ".join(missing_ref))
        if missing_candidate:
            details.append("candidate missing " + ", ".join(missing_candidate))
        raise TraceError("; ".join(details))

    columns = [
        field
        for field in columns
        if field in reference_fields and field in candidate_fields
    ]
    if not columns:
        raise TraceError("no comparable columns remain")
    return columns


def compare(
    reference_rows: dict[int, dict[str, str]],
    candidate_rows: dict[int, dict[str, str]],
    columns: Sequence[str],
    limit: int,
) -> tuple[list[dict[str, Any]], int, int]:
    differences: list[dict[str, Any]] = []
    compared_frames = 0
    missing_frames = 0
    ref_frames = set(reference_rows)
    candidate_frames = set(candidate_rows)

    for frame in sorted(ref_frames | candidate_frames):
        if frame not in candidate_frames:
            missing_frames += 1
            if len(differences) < limit:
                differences.append(
                    {
                        "frame": frame,
                        "column": "*",
                        "reference": "present",
                        "candidate": "missing",
                        "kind": "frame",
                    }
                )
            continue
        if frame not in ref_frames:
            missing_frames += 1
            if len(differences) < limit:
                differences.append(
                    {
                        "frame": frame,
                        "column": "*",
                        "reference": "missing",
                        "candidate": "present",
                        "kind": "frame",
                    }
                )
            continue

        compared_frames += 1
        for column in columns:
            reference = normalize(reference_rows[frame].get(column), column)
            candidate = normalize(candidate_rows[frame].get(column), column)
            if reference != candidate and len(differences) < limit:
                differences.append(
                    {
                        "frame": frame,
                        "column": column,
                        "reference": reference,
                        "candidate": candidate,
                        "kind": "value",
                    }
                )

    return differences, compared_frames, missing_frames


def print_report(report: dict[str, Any]) -> None:
    print(f"REFERENCE_FRAMES={report['reference_frames']}")
    print(f"CANDIDATE_FRAMES={report['candidate_frames']}")
    print(f"COMPARED_FRAMES={report['compared_frames']}")
    print(f"MISSING_FRAMES={report['missing_frames']}")
    print("COLUMNS=" + ",".join(report["columns"]))
    print("RESULT=" + ("MATCH" if report["match"] else "DIFFERENT"))
    first = report["first_divergence"]
    if first:
        print(f"FIRST_DIVERGENCE_FRAME={first['frame']}")
        print(f"FIRST_DIVERGENCE_COLUMN={first['column']}")
        print(f"REFERENCE_VALUE={first['reference']}")
        print(f"CANDIDATE_VALUE={first['candidate']}")
    for item in report["mismatches"]:
        print(
            f"MISMATCH frame={item['frame']} column={item['column']} "
            f"reference={item['reference']} candidate={item['candidate']}"
        )


def self_test() -> int:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        reference = root / "reference.csv"
        equal = root / "equal.csv"
        different = root / "different.csv"
        reference.write_text(
            "frame,state_hash,score,rng_seed\n"
            "0,00000000000000aa,0,$1234\n"
            "1,00000000000000bb,40,$5678\n",
            encoding="utf-8",
        )
        equal.write_text(
            "frame,state_hash,score,rng_seed\n"
            "0,0xaa,0,0x1234\n"
            "1,0xbb,40,0x5678\n",
            encoding="utf-8",
        )
        different.write_text(
            "frame,state_hash,score,rng_seed\n"
            "0,0xab,0,0x1234\n"
            "2,0xbc,41,0x5678\n",
            encoding="utf-8",
        )

        ref_fields, ref_rows = load_trace(reference, "frame")
        equal_fields, equal_rows = load_trace(equal, "frame")
        columns = comparable_columns(
            ref_fields, equal_fields, None, DEFAULT_IGNORED, "frame", False
        )
        differences, compared, missing = compare(ref_rows, equal_rows, columns, 20)
        if differences or compared != 2 or missing:
            raise TraceError("equal-trace self-test failed")

        diff_fields, diff_rows = load_trace(different, "frame")
        columns = comparable_columns(
            ref_fields, diff_fields, None, DEFAULT_IGNORED, "frame", False
        )
        differences, compared, missing = compare(ref_rows, diff_rows, columns, 20)
        if (
            not differences
            or differences[0]["frame"] != 0
            or compared != 1
            or missing != 2
        ):
            raise TraceError("chronological divergence self-test failed")

    print("trace_compare self-test: OK")
    return EXIT_MATCH


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compare two frame-indexed CSV traces."
    )
    parser.add_argument("reference", nargs="?", type=Path)
    parser.add_argument("candidate", nargs="?", type=Path)
    parser.add_argument("--columns")
    parser.add_argument("--ignore", default=",".join(sorted(DEFAULT_IGNORED)))
    parser.add_argument("--frame-column", default="frame")
    parser.add_argument("--allow-missing-columns", action="store_true")
    parser.add_argument("--max-mismatches", type=int, default=20)
    parser.add_argument("--json", dest="json_path", type=Path)
    parser.add_argument("--self-test", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            return self_test()
        if args.reference is None or args.candidate is None:
            parser.error("reference and candidate CSV files are required")
        if args.max_mismatches < 1:
            raise TraceError("--max-mismatches must be at least 1")

        ref_fields, ref_rows = load_trace(args.reference, args.frame_column)
        candidate_fields, candidate_rows = load_trace(
            args.candidate, args.frame_column
        )
        columns = comparable_columns(
            ref_fields,
            candidate_fields,
            parse_list(args.columns),
            parse_list(args.ignore) or [],
            args.frame_column,
            args.allow_missing_columns,
        )
        differences, compared, missing = compare(
            ref_rows, candidate_rows, columns, args.max_mismatches
        )
        report = {
            "match": not differences and missing == 0,
            "reference": str(args.reference),
            "candidate": str(args.candidate),
            "reference_frames": len(ref_rows),
            "candidate_frames": len(candidate_rows),
            "compared_frames": compared,
            "missing_frames": missing,
            "columns": columns,
            "first_divergence": differences[0] if differences else None,
            "mismatches": differences,
        }
        print_report(report)
        if args.json_path:
            args.json_path.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
        return EXIT_MATCH if report["match"] else EXIT_DIFFERENT
    except TraceError as exc:
        print(f"trace_compare: {exc}", file=sys.stderr)
        return EXIT_USAGE
    except OSError as exc:
        print(f"trace_compare: {exc}", file=sys.stderr)
        return EXIT_USAGE


if __name__ == "__main__":
    raise SystemExit(main())
