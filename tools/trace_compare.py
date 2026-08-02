#!/usr/bin/env python3
"""Compare deterministic Tetris NES frame traces.

The tool is intentionally ROM-agnostic: it compares CSV state exported by the
native port with CSV state captured from an emulator without embedding or
extracting copyrighted game data.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Sequence

DEFAULT_IGNORED = {"comment", "notes"}
EXIT_MATCH = 0
EXIT_DIFFERENT = 1
EXIT_USAGE = 2


class TraceError(ValueError):
    """Raised for malformed or incompatible traces."""


@dataclass(frozen=True)
class Mismatch:
    frame: int
    column: str
    reference: Any
    candidate: Any
    kind: str = "value"


@dataclass
class Trace:
    path: Path
    fields: list[str]
    rows: dict[int, dict[str, str]]


def _parse_columns(value: str | None) -> list[str] | None:
    if value is None:
        return None
    columns = [item.strip() for item in value.split(",") if item.strip()]
    if not columns:
        raise TraceError("column list is empty")
    return columns


def _parse_int(text: str, column: str) -> int | None:
    value = text.strip().lower().replace("_", "")
    if value == "":
        return None
    if value in {"true", "yes", "on"}:
        return 1
    if value in {"false", "no", "off"}:
        return 0
    try:
        if value.startswith("$"):
            return int(value[1:], 16)
        if value.startswith("0x"):
            return int(value[2:], 16)
        if value.startswith("%"):
            return int(value[1:], 2)
        if value.startswith("0b"):
            return int(value[2:], 2)
        if value.startswith("-"):
            return int(value, 10)
        if value.isdigit():
            return int(value, 10)
        if "hash" in column.lower() and all(ch in "0123456789abcdef" for ch in value):
            return int(value, 16)
    except ValueError:
        return None
    return None


def normalize(text: str | None, column: str) -> Any:
    if text is None:
        return None
    stripped = text.strip()
    numeric = _parse_int(stripped, column)
    if numeric is not None:
        return numeric
    return stripped.casefold()


def load_trace(path: Path, frame_column: str) -> Trace:
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
        fields = [field.strip() for field in reader.fieldnames if field is not None]
        if frame_column not in fields:
            raise TraceError(f"{path}: missing frame column {frame_column!r}")

        rows: dict[int, dict[str, str]] = {}
        for row_index, raw in enumerate(reader, start=2):
            frame_text = raw.get(frame_column)
            frame_value = _parse_int(frame_text or "", frame_column)
            if frame_value is None or frame_value < 0:
                raise TraceError(
                    f"{path}:{row_index}: invalid {frame_column} value {frame_text!r}"
                )
            if frame_value in rows:
                raise TraceError(f"{path}:{row_index}: duplicate frame {frame_value}")
            rows[frame_value] = {
                key.strip(): (value or "").strip()
                for key, value in raw.items()
                if key is not None
            }

    if not rows:
        raise TraceError(f"{path}: trace contains no data rows")
    return Trace(path=path, fields=fields, rows=rows)


def choose_columns(
    reference: Trace,
    candidate: Trace,
    requested: Sequence[str] | None,
    ignored: Iterable[str],
    frame_column: str,
    allow_missing_columns: bool,
) -> list[str]:
    ignored_set = set(ignored) | {frame_column}
    if requested is None:
        columns = [
            field
            for field in reference.fields
            if field in candidate.fields and field not in ignored_set
        ]
    else:
        columns = [field for field in requested if field not in ignored_set]

    missing_reference = [field for field in columns if field not in reference.fields]
    missing_candidate = [field for field in columns if field not in candidate.fields]
    if (missing_reference or missing_candidate) and not allow_missing_columns:
        details = []
        if missing_reference:
            details.append("reference missing " + ", ".join(missing_reference))
        if missing_candidate:
            details.append("candidate missing " + ", ".join(missing_candidate))
        raise TraceError("; ".join(details))

    columns = [
        field
        for field in columns
        if field in reference.fields and field in candidate.fields
    ]
    if not columns:
        raise TraceError("no comparable columns remain")
    return columns


def compare_traces(
    reference: Trace,
    candidate: Trace,
    columns: Sequence[str],
    max_mismatches: int,
) -> tuple[list[Mismatch], int, int]:
    mismatches: list[Mismatch] = []
    reference_frames = set(reference.rows)
    candidate_frames = set(candidate.rows)
    missing_frames = 0

    for frame in sorted(reference_frames - candidate_frames):
        missing_frames += 1
        if len(mismatches) < max_mismatches:
            mismatches.append(Mismatch(frame, "*", "present", "missing", "frame"))

    for frame in sorted(candidate_frames - reference_frames):
        missing_frames += 1
        if len(mismatches) < max_mismatches:
            mismatches.append(Mismatch(frame, "*", "missing", "present", "frame"))

    compared_frames = 0
    for frame in sorted(reference_frames & candidate_frames):
        compared_frames += 1
        ref_row = reference.rows[frame]
        cand_row = candidate.rows[frame]
        for column in columns:
            ref_value = normalize(ref_row.get(column), column)
            cand_value = normalize(cand_row.get(column), column)
            if ref_value != cand_value and len(mismatches) < max_mismatches:
                mismatches.append(
                    Mismatch(frame, column, ref_value, cand_value, "value")
                )

    return mismatches, compared_frames, missing_frames


def make_report(
    reference: Trace,
    candidate: Trace,
    columns: Sequence[str],
    mismatches: Sequence[Mismatch],
    compared_frames: int,
    missing_frames: int,
) -> dict[str, Any]:
    return {
        "match": not mismatches and missing_frames == 0,
        "reference": str(reference.path),
        "candidate": str(candidate.path),
        "reference_frames": len(reference.rows),
        "candidate_frames": len(candidate.rows),
        "compared_frames": compared_frames,
        "missing_frames": missing_frames,
        "columns": list(columns),
        "first_divergence": (
            {
                "frame": mismatches[0].frame,
                "column": mismatches[0].column,
                "reference": mismatches[0].reference,
                "candidate": mismatches[0].candidate,
                "kind": mismatches[0].kind,
            }
            if mismatches
            else None
        ),
        "mismatches": [
            {
                "frame": item.frame,
                "column": item.column,
                "reference": item.reference,
                "candidate": item.candidate,
                "kind": item.kind,
            }
            for item in mismatches
        ],
    }


def print_human(report: dict[str, Any]) -> None:
    print(f"REFERENCE_FRAMES={report['reference_frames']}")
    print(f"CANDIDATE_FRAMES={report['candidate_frames']}")
    print(f"COMPARED_FRAMES={report['compared_frames']}")
    print(f"MISSING_FRAMES={report['missing_frames']}")
    print("COLUMNS=" + ",".join(report["columns"]))
    if report["match"]:
        print("RESULT=MATCH")
        return

    print("RESULT=DIFFERENT")
    first = report["first_divergence"]
    if first:
        print(f"FIRST_DIVERGENCE_FRAME={first['frame']}")
        print(f"FIRST_DIVERGENCE_COLUMN={first['column']}")
        print(f"REFERENCE_VALUE={first['reference']}")
        print(f"CANDIDATE_VALUE={first['candidate']}")
    for item in report["mismatches"]:
        print(
            "MISMATCH "
            f"frame={item['frame']} column={item['column']} "
            f"reference={item['reference']} candidate={item['candidate']}"
        )


def run_self_test() -> int:
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
            "0,0xaa,0,0x1234\n"
            "1,0xbc,41,0x5678\n",
            encoding="utf-8",
        )

        ref_trace = load_trace(reference, "frame")
        equal_trace = load_trace(equal, "frame")
        diff_trace = load_trace(different, "frame")
        columns = choose_columns(
            ref_trace, equal_trace, None, DEFAULT_IGNORED, "frame", False
        )
        mismatches, compared, missing = compare_traces(
            ref_trace, equal_trace, columns, 20
        )
        if mismatches or missing or compared != 2:
            raise TraceError("equal-trace self-test failed")

        columns = choose_columns(
            ref_trace, diff_trace, None, DEFAULT_IGNORED, "frame", False
        )
        mismatches, compared, missing = compare_traces(
            ref_trace, diff_trace, columns, 20
        )
        if not mismatches or mismatches[0].frame != 1 or compared != 2 or missing:
            raise TraceError("divergence self-test failed")

    print("trace_compare self-test: OK")
    return EXIT_MATCH


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compare two frame-indexed CSV traces and report the first divergence."
        )
    )
    parser.add_argument("reference", nargs="?", type=Path)
    parser.add_argument("candidate", nargs="?", type=Path)
    parser.add_argument(
        "--columns",
        help="comma-separated columns to compare; default is the common schema",
    )
    parser.add_argument(
        "--ignore",
        default=",".join(sorted(DEFAULT_IGNORED)),
        help="comma-separated columns to ignore",
    )
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
            return run_self_test()
        if args.reference is None or args.candidate is None:
            parser.error("reference and candidate CSV files are required")
        if args.max_mismatches < 1:
            raise TraceError("--max-mismatches must be at least 1")

        requested = _parse_columns(args.columns)
        ignored = _parse_columns(args.ignore) or []
        reference = load_trace(args.reference, args.frame_column)
        candidate = load_trace(args.candidate, args.frame_column)
        columns = choose_columns(
            reference,
            candidate,
            requested,
            ignored,
            args.frame_column,
            args.allow_missing_columns,
        )
        mismatches, compared_frames, missing_frames = compare_traces(
            reference, candidate, columns, args.max_mismatches
        )
        report = make_report(
            reference,
            candidate,
            columns,
            mismatches,
            compared_frames,
            missing_frames,
        )
        print_human(report)
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
