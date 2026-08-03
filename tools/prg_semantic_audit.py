#!/usr/bin/env python3
"""Verify complete semantic classification of generic PRG/source labels."""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import re
import sys

NAME_RE = re.compile(r"^[a-z][a-z0-9_]*$")


def load_tsv(paths: list[Path]) -> tuple[dict[str, dict[str, str]], list[str]]:
    entries: dict[str, dict[str, str]] = {}
    errors: list[str] = []
    for path in paths:
        with path.open(encoding="utf-8", newline="") as handle:
            for line_number, row in enumerate(csv.reader(handle, delimiter="\t"), 1):
                if not row or row[0].startswith("#"):
                    continue
                if len(row) != 5:
                    errors.append(f"{path}:{line_number}: expected 5 tab-separated fields")
                    continue
                label, semantic_name, kind, subsystem, evidence = (item.strip() for item in row)
                if label in entries:
                    errors.append(f"duplicate label classification: {label}")
                    continue
                entries[label] = {
                    "label": label,
                    "semantic_name": semantic_name,
                    "kind": kind,
                    "subsystem": subsystem,
                    "evidence": evidence,
                    "manifest_file": str(path),
                    "manifest_line": str(line_number),
                }
    return entries, errors


def audit(context_path: Path, tsv_paths: list[Path], revision: str) -> dict[str, object]:
    context = json.loads(context_path.read_text(encoding="utf-8"))
    expected = {str(item["name"]) for item in context["items"]}
    entries, errors = load_tsv(tsv_paths)
    actual = set(entries)
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    aliases: dict[str, str] = {}
    duplicate_aliases: list[dict[str, str]] = []
    invalid_entries: list[dict[str, str]] = []

    for label, entry in sorted(entries.items()):
        alias = entry["semantic_name"]
        if alias in aliases:
            duplicate_aliases.append(
                {"semantic_name": alias, "first_label": aliases[alias], "second_label": label}
            )
        else:
            aliases[alias] = label
        reasons: list[str] = []
        if not NAME_RE.match(alias):
            reasons.append("semantic name is not lower_snake_case")
        if alias.lower() == label.lower() or alias.startswith("unknown_"):
            reasons.append("semantic name does not add meaning")
        if not entry["kind"]:
            reasons.append("kind is empty")
        if not entry["subsystem"]:
            reasons.append("subsystem is empty")
        if len(entry["evidence"]) < 12:
            reasons.append("evidence is too short")
        if reasons:
            invalid_entries.append({"label": label, "reasons": "; ".join(reasons)})

    classified = len(expected) - len(missing)
    percent = round(classified * 100.0 / len(expected), 4) if expected else 0.0
    passed = not (errors or missing or extra or duplicate_aliases or invalid_entries)
    by_subsystem: dict[str, int] = {}
    by_kind: dict[str, int] = {}
    for entry in entries.values():
        by_subsystem[entry["subsystem"]] = by_subsystem.get(entry["subsystem"], 0) + 1
        by_kind[entry["kind"]] = by_kind.get(entry["kind"], 0) + 1
    return {
        "schema": 1,
        "source_revision": revision,
        "context_source_revision": context.get("source_revision"),
        "expected_generic_labels": len(expected),
        "classified_labels": classified,
        "semantic_classification_percent": percent,
        "gate_passed": passed,
        "missing_labels": missing,
        "extra_labels": extra,
        "duplicate_aliases": duplicate_aliases,
        "invalid_entries": invalid_entries,
        "parse_errors": errors,
        "by_subsystem": dict(sorted(by_subsystem.items())),
        "by_kind": dict(sorted(by_kind.items())),
        "classifications": [entries[label] for label in sorted(entries)],
        "interpretation": (
            "100% means every generic label in the pinned ld65/source inventory has a "
            "unique semantic alias, type, subsystem and evidence. It does not change the runtime."
        ),
    }


def self_test() -> int:
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        context = root / "context.json"
        manifest = root / "symbols.tsv"
        context.write_text(
            json.dumps({"source_revision": "x", "items": [{"name": "L8000"}]}),
            encoding="utf-8",
        )
        manifest.write_text(
            "# label\tsemantic_name\tkind\tsubsystem\tevidence\n"
            "L8000\treset_dispatch_loop\tloop_entry\tboot\tsample.asm:1; test evidence.\n",
            encoding="utf-8",
        )
        report = audit(context, [manifest], "x")
        if not report["gate_passed"] or report["semantic_classification_percent"] != 100:
            print(json.dumps(report, indent=2), file=sys.stderr)
            return 1
    print("prg_semantic_audit self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("context", type=Path, nargs="?")
    parser.add_argument("manifests", type=Path, nargs="*")
    parser.add_argument("--source-revision", default="unknown")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.context is None or not args.manifests:
        parser.error("context report and TSV manifests are required")
    try:
        report = audit(args.context, args.manifests, args.source_revision)
    except (OSError, ValueError, TypeError, KeyError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "PRG semantic audit: "
        f"{report['semantic_classification_percent']}% "
        f"classified={report['classified_labels']}/{report['expected_generic_labels']}"
    )
    if not report["gate_passed"]:
        for key in ("parse_errors", "missing_labels", "extra_labels", "duplicate_aliases", "invalid_entries"):
            if report[key]:
                print(f"{key}: {report[key]}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
