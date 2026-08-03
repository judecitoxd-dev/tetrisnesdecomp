#!/usr/bin/env python3
"""Audit named 6502 control-flow edges from ca65 source without ROM payloads."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Iterable

BRANCHES = {"bcc", "bcs", "beq", "bmi", "bne", "bpl", "bvc", "bvs"}
DIRECT = BRANCHES | {"jsr", "jmp"}
LABEL_RE = re.compile(r"^\s*([A-Za-z_.$@][A-Za-z0-9_.$@]*)\s*:")
INSTRUCTION_RE = re.compile(r"^\s*([A-Za-z]{3})\b\s*(.*)$")
SYMBOL_RE = re.compile(r"[A-Za-z_.$@][A-Za-z0-9_.$@]*")
GENERIC_RE = re.compile(r"^(?:L[0-9A-Fa-f]{4}|LE[0-9A-Fa-f]{3}|unreferenced_.*)$")
ANONYMOUS = {":+", ":-", ":++", ":--"}


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


def qualify(name: str, current_global: str | None) -> str:
    if name.startswith("@") and current_global:
        return f"{current_global}::{name}"
    return name


def split_label_and_tail(line: str) -> tuple[list[str], str]:
    labels: list[str] = []
    tail = line
    while True:
        match = LABEL_RE.match(tail)
        if not match:
            break
        labels.append(match.group(1))
        tail = tail[match.end():]
    return labels, tail


def normalize_operand(operand: str, current_global: str | None) -> tuple[str, bool]:
    value = operand.strip()
    if not value:
        return "", False
    if value.startswith("(") and value.endswith(")"):
        inner = value[1:-1].strip()
        return qualify(inner, current_global), True
    value = value.split(",", 1)[0].strip()
    value = value.lstrip("#<>")
    value = re.split(r"[+\-*/]", value, maxsplit=1)[0].strip()
    if value in ANONYMOUS:
        return value, False
    if value.startswith("$") or value.startswith("%") or value[:1].isdigit():
        return value, False
    match = SYMBOL_RE.match(value)
    if not match:
        return value, False
    return qualify(match.group(0), current_global), False


def collect_definitions(paths: Iterable[Path]) -> tuple[set[str], dict[str, dict[str, object]]]:
    definitions: set[str] = set()
    metadata: dict[str, dict[str, object]] = {}
    for path in paths:
        current_global: str | None = None
        for line_number, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            line = strip_comment(raw)
            labels, _ = split_label_and_tail(line)
            for label in labels:
                if label.startswith("@"):
                    qualified = qualify(label, current_global)
                else:
                    current_global = label
                    qualified = label
                definitions.add(qualified)
                metadata.setdefault(qualified, {"file": str(path), "line": line_number})
    return definitions, metadata


def load_ld65_labels(path: Path | None) -> set[str]:
    if path is None:
        return set()
    result: set[str] = set()
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = raw.split()
        if len(parts) >= 3:
            result.add(parts[2].lstrip("."))
    return result


def audit(paths: list[Path], labels_path: Path | None, revision: str) -> dict[str, object]:
    definitions, definition_metadata = collect_definitions(paths)
    ld65_labels = load_ld65_labels(labels_path)
    global_symbols = definitions | ld65_labels
    edges: list[dict[str, object]] = []
    table_refs: list[dict[str, object]] = []
    indirect_sites: list[dict[str, object]] = []
    routines_with_instruction: set[str] = set()

    for path in paths:
        current_global: str | None = None
        current_label: str | None = None
        for line_number, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            line = strip_comment(raw)
            if not line.strip():
                continue
            labels, tail = split_label_and_tail(line)
            for label in labels:
                if label.startswith("@"):
                    current_label = qualify(label, current_global)
                else:
                    current_global = label
                    current_label = label
            statement = tail.strip() if labels else line.strip()
            if not statement:
                continue
            if statement.lower().startswith(".addr"):
                payload = statement[5:].strip()
                for raw_target in payload.split(","):
                    target, indirect = normalize_operand(raw_target, current_global)
                    if not target:
                        continue
                    resolved = (
                        target in global_symbols
                        or target in ANONYMOUS
                        or target.startswith("$")
                        or target[:1].isdigit()
                    )
                    table_refs.append({
                        "source": current_label or current_global or "<file>",
                        "target": target,
                        "resolved": resolved,
                        "file": str(path),
                        "line": line_number,
                        "indirect_syntax": indirect,
                    })
                continue
            match = INSTRUCTION_RE.match(statement)
            if not match:
                continue
            mnemonic = match.group(1).lower()
            operand = match.group(2).strip()
            if current_global:
                routines_with_instruction.add(current_global)
            if mnemonic not in DIRECT:
                continue
            target, indirect = normalize_operand(operand, current_global)
            source = current_label or current_global or "<file>"
            if mnemonic == "jmp" and indirect:
                indirect_sites.append({
                    "source": source,
                    "operand": target,
                    "file": str(path),
                    "line": line_number,
                    "classification": "computed_jump",
                })
                continue
            resolved = (
                target in global_symbols
                or target in ANONYMOUS
                or target.startswith("$")
                or target.startswith("%")
                or target[:1].isdigit()
            )
            edges.append({
                "kind": mnemonic,
                "source": source,
                "target": target,
                "resolved": resolved,
                "file": str(path),
                "line": line_number,
            })

    direct_total = len(edges)
    direct_resolved = sum(1 for edge in edges if edge["resolved"])
    table_total = len(table_refs)
    table_resolved = sum(1 for edge in table_refs if edge["resolved"])
    indirect_total = len(indirect_sites)
    # Every computed jump remains a separately named audit item. None is silently counted.
    classified_indirect = sum(1 for site in indirect_sites if site["classification"])
    denominator = direct_total + table_total + indirect_total
    numerator = direct_resolved + table_resolved + classified_indirect
    percent = round(numerator * 100.0 / denominator, 4) if denominator else 0.0
    generic_routines = sorted(name for name in routines_with_instruction if GENERIC_RE.match(name))
    return {
        "schema": 1,
        "source_revision": revision,
        "source_files": [str(path) for path in paths],
        "definitions_total": len(definitions),
        "ld65_labels_total": len(ld65_labels),
        "routines_with_instructions": len(routines_with_instruction),
        "generic_routine_labels": generic_routines,
        "generic_routine_count": len(generic_routines),
        "direct_edges": {
            "total": direct_total,
            "resolved": direct_resolved,
            "unresolved": direct_total - direct_resolved,
            "unresolved_items": [edge for edge in edges if not edge["resolved"]],
        },
        "address_table_references": {
            "total": table_total,
            "resolved": table_resolved,
            "unresolved": table_total - table_resolved,
            "unresolved_items": [edge for edge in table_refs if not edge["resolved"]],
        },
        "computed_jump_sites": {
            "total": indirect_total,
            "classified": classified_indirect,
            "items": indirect_sites,
        },
        "static_control_flow_audit_percent": percent,
        "gate_passed": (
            direct_resolved == direct_total
            and table_resolved == table_total
            and classified_indirect == indirect_total
        ),
        "interpretation": (
            "This is a source-level edge inventory. Dynamic dispatch meaning and state "
            "contracts are audited separately."
        ),
    }


def self_test() -> int:
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "sample.asm"
        path.write_text(
            "start: jsr worker\n"
            "       beq @done\n"
            "       .addr worker, @done\n"
            "@done: rts\n"
            "worker: jmp start\n",
            encoding="utf-8",
        )
        report = audit([path], None, "self-test")
        if not report["gate_passed"] or report["static_control_flow_audit_percent"] != 100:
            print(json.dumps(report, indent=2), file=sys.stderr)
            return 1
    print("prg_control_flow_audit self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("sources", type=Path, nargs="*")
    parser.add_argument("--labels", type=Path)
    parser.add_argument("--source-revision", default="unknown")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.sources:
        parser.error("at least one source file is required")
    try:
        report = audit(args.sources, args.labels, args.source_revision)
    except OSError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "PRG control-flow audit: "
        f"{report['static_control_flow_audit_percent']}% "
        f"direct={report['direct_edges']['resolved']}/{report['direct_edges']['total']} "
        f"tables={report['address_table_references']['resolved']}/{report['address_table_references']['total']} "
        f"computed={report['computed_jump_sites']['classified']}/{report['computed_jump_sites']['total']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
