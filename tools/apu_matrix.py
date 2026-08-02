#!/usr/bin/env python3
"""Capture and compare the complete original Tetris NES APU scenario matrix."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

TRACKS = tuple(range(1, 11))
SCENARIOS = (
    "move", "rotate", "lock", "line", "tetris", "level",
    "game-over", "complete",
)
DEFAULT_COLUMNS = (
    "cpu_cycles", "driver_cycles", "dmc_stall_cycles", "irq", "apu_writes"
)


class MatrixError(RuntimeError):
    pass


@dataclass(frozen=True)
class Difference:
    case: str
    frame: int
    column: str
    reference: str
    candidate: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(command: Sequence[str]) -> None:
    completed = subprocess.run(command, text=True, capture_output=True)
    if completed.returncode != 0:
        raise MatrixError(
            "command failed: " + " ".join(command) + "\n" +
            completed.stdout + completed.stderr
        )


def read_trace(path: Path) -> tuple[list[str], dict[int, dict[str, str]]]:
    try:
        handle = path.open("r", encoding="utf-8-sig", newline="")
    except OSError as exc:
        raise MatrixError(f"cannot open {path}: {exc}") from exc
    with handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames or "frame" not in reader.fieldnames:
            raise MatrixError(f"{path}: missing frame-indexed CSV header")
        fields = [field.strip() for field in reader.fieldnames]
        rows: dict[int, dict[str, str]] = {}
        for line, raw in enumerate(reader, start=2):
            try:
                frame = int((raw.get("frame") or "").strip(), 0)
            except ValueError as exc:
                raise MatrixError(f"{path}:{line}: invalid frame") from exc
            if frame in rows:
                raise MatrixError(f"{path}:{line}: duplicate frame {frame}")
            rows[frame] = {
                key.strip(): (value or "").strip()
                for key, value in raw.items() if key is not None
            }
    if not rows:
        raise MatrixError(f"{path}: empty trace")
    return fields, rows


def case_names() -> Iterable[str]:
    for track in TRACKS:
        yield f"track-{track:02d}"
    for scenario in SCENARIOS:
        yield f"effect-{scenario}"


def summarize_trace(path: Path) -> dict[str, object]:
    fields, rows = read_trace(path)
    first = min(rows)
    last = max(rows)
    writes = sum(1 for row in rows.values() if row.get("apu_writes"))
    cycles = sum(int(row.get("cpu_cycles", "0") or "0", 0) for row in rows.values())
    return {
        "file": path.name,
        "sha256": sha256(path),
        "frames": len(rows),
        "first_frame": first,
        "last_frame": last,
        "frames_with_writes": writes,
        "cpu_cycles": cycles,
        "columns": fields,
    }


def capture(args: argparse.Namespace) -> int:
    output: Path = args.output
    output.mkdir(parents=True, exist_ok=True)
    manifest: dict[str, object] = {
        "schema": 1,
        "tracks": {},
        "effects": {},
    }
    for track in TRACKS:
        stem = f"track-{track:02d}"
        wav = output / f"{stem}.wav"
        trace = output / f"{stem}.csv"
        run([
            str(args.apu_render), str(args.rom), str(track),
            str(args.seconds), str(wav), str(trace),
        ])
        manifest["tracks"][str(track)] = {
            "wav": {"file": wav.name, "sha256": sha256(wav)},
            "trace": summarize_trace(trace),
        }
        if not args.keep_wav:
            wav.unlink()
            manifest["tracks"][str(track)]["wav"]["removed_after_hash"] = True

    for scenario in SCENARIOS:
        trace = output / f"effect-{scenario}.csv"
        run([
            str(args.apu_scenario), str(args.rom), scenario,
            str(args.effect_frames), str(trace),
        ])
        manifest["effects"][scenario] = summarize_trace(trace)

    manifest_path = output / "apu-matrix.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(f"MATRIX={manifest_path}")
    print(f"TRACKS={len(TRACKS)}")
    print(f"EFFECTS={len(SCENARIOS)}")
    return 0


def compare_case(reference: Path, candidate: Path, case: str,
                 columns: Sequence[str]) -> Difference | None:
    ref_fields, ref_rows = read_trace(reference)
    cand_fields, cand_rows = read_trace(candidate)
    for column in columns:
        if column not in ref_fields or column not in cand_fields:
            raise MatrixError(f"{case}: missing comparison column {column}")
    all_frames = sorted(set(ref_rows) | set(cand_rows))
    for frame in all_frames:
        if frame not in ref_rows:
            return Difference(case, frame, "*", "missing", "present")
        if frame not in cand_rows:
            return Difference(case, frame, "*", "present", "missing")
        for column in columns:
            reference_value = ref_rows[frame].get(column, "").strip().upper()
            candidate_value = cand_rows[frame].get(column, "").strip().upper()
            if reference_value != candidate_value:
                return Difference(case, frame, column,
                                  reference_value, candidate_value)
    return None


def compare(args: argparse.Namespace) -> int:
    columns = tuple(item.strip() for item in args.columns.split(",") if item.strip())
    if not columns:
        raise MatrixError("comparison column list is empty")
    differences: list[Difference] = []
    compared = 0
    for case in case_names():
        reference = args.reference / f"{case}.csv"
        candidate = args.candidate / f"{case}.csv"
        if not reference.is_file() or not candidate.is_file():
            if args.allow_missing:
                continue
            raise MatrixError(f"missing matrix case: {case}")
        difference = compare_case(reference, candidate, case, columns)
        compared += 1
        if difference:
            differences.append(difference)
            print(
                f"DIFFERENT case={case} frame={difference.frame} "
                f"column={difference.column} reference={difference.reference} "
                f"candidate={difference.candidate}"
            )
        else:
            print(f"MATCH case={case}")

    report = {
        "match": not differences,
        "compared_cases": compared,
        "expected_cases": len(tuple(case_names())),
        "columns": list(columns),
        "differences": [difference.__dict__ for difference in differences],
    }
    if args.json:
        args.json.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    print(f"RESULT={'MATCH' if not differences else 'DIFFERENT'}")
    print(f"COMPARED_CASES={compared}")
    return 0 if not differences else 1


def write_test_trace(path: Path, changed: bool = False) -> None:
    path.write_text(
        "frame,cpu_cycles,driver_cycles,dmc_stall_cycles,irq,apu_writes\n"
        "0,29780,20,0,0,4000=30\n"
        f"1,29781,21,0,0,{'4002=AB' if changed else '4002=AA'}\n",
        encoding="utf-8",
    )


def self_test() -> int:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        reference = root / "reference.csv"
        equal = root / "equal.csv"
        changed = root / "changed.csv"
        write_test_trace(reference)
        write_test_trace(equal)
        write_test_trace(changed, changed=True)
        if compare_case(reference, equal, "self", DEFAULT_COLUMNS) is not None:
            raise MatrixError("equal self-test failed")
        difference = compare_case(reference, changed, "self", DEFAULT_COLUMNS)
        if not difference or difference.frame != 1 or difference.column != "apu_writes":
            raise MatrixError("difference self-test failed")
        if len(tuple(case_names())) != 18:
            raise MatrixError("scenario matrix size changed unexpectedly")
    print("apu_matrix self-test: OK cases=18")
    return 0


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description="Capture or compare all original Tetris NES APU tracks/effects."
    )
    sub = result.add_subparsers(dest="command", required=True)

    capture_parser = sub.add_parser("capture")
    capture_parser.add_argument("--apu-render", type=Path, required=True)
    capture_parser.add_argument("--apu-scenario", type=Path, required=True)
    capture_parser.add_argument("--rom", type=Path, required=True)
    capture_parser.add_argument("--output", type=Path, required=True)
    capture_parser.add_argument("--seconds", type=float, default=2.0)
    capture_parser.add_argument("--effect-frames", type=int, default=180)
    capture_parser.add_argument("--keep-wav", action="store_true")
    capture_parser.set_defaults(function=capture)

    compare_parser = sub.add_parser("compare")
    compare_parser.add_argument("reference", type=Path)
    compare_parser.add_argument("candidate", type=Path)
    compare_parser.add_argument("--columns", default=",".join(DEFAULT_COLUMNS))
    compare_parser.add_argument("--allow-missing", action="store_true")
    compare_parser.add_argument("--json", type=Path)
    compare_parser.set_defaults(function=compare)

    test_parser = sub.add_parser("self-test")
    test_parser.set_defaults(function=lambda _: self_test())
    return result


def main(argv: Sequence[str] | None = None) -> int:
    try:
        args = parser().parse_args(argv)
        return int(args.function(args))
    except MatrixError as exc:
        print(f"apu_matrix: {exc}", file=sys.stderr)
        return 2
    except OSError as exc:
        print(f"apu_matrix: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
