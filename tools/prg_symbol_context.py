#!/usr/bin/env python3
"""Build a metadata-only context inventory for remaining generic PRG labels."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

GENERIC_RE = re.compile(r"^(?:L[0-9A-Fa-f]{4}|LE[0-9A-Fa-f]{3}|unreferenced_.*|__.*|\.size|\.bank)$")
LABEL_RE = re.compile(r"^\s*([A-Za-z_.$@][A-Za-z0-9_.$@]*)\s*:")
INSTRUCTION_RE = re.compile(r"^\s*([A-Za-z]{3})\b")
SEMANTIC_RE = re.compile(r"^(?!L[0-9A-Fa-f]{4}$)(?!LE[0-9A-Fa-f]{3}$)(?!unreferenced_).+")


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


def load_labels(path: Path) -> dict[str, list[int]]:
    result: dict[str, set[int]] = {}
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = raw.split()
        if len(parts) < 3:
            continue
        try:
            address = int(parts[1], 16)
        except ValueError:
            continue
        name = parts[2].lstrip(".")
        result.setdefault(name, set()).add(address)
    return {name: sorted(addresses) for name, addresses in result.items()}


def source_definitions(paths: list[Path]) -> tuple[dict[str, list[dict[str, object]]], dict[str, list[str]]]:
    definitions: dict[str, list[dict[str, object]]] = {}
    contents: dict[str, list[str]] = {}
    for path in paths:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        contents[str(path)] = lines
        for index, raw in enumerate(lines):
            match = LABEL_RE.match(strip_comment(raw))
            if not match:
                continue
            name = match.group(1)
            if name.startswith("@"):
                continue
            definitions.setdefault(name, []).append(
                {"file": str(path), "line": index + 1, "index": index}
            )
    return definitions, contents


def nearest_global_labels(lines: list[str], index: int) -> tuple[str | None, str | None]:
    previous = None
    following = None
    for cursor in range(index - 1, -1, -1):
        match = LABEL_RE.match(strip_comment(lines[cursor]))
        if match and not match.group(1).startswith("@"):
            previous = match.group(1)
            break
    for cursor in range(index + 1, len(lines)):
        match = LABEL_RE.match(strip_comment(lines[cursor]))
        if match and not match.group(1).startswith("@"):
            following = match.group(1)
            break
    return previous, following


def preceding_comments(lines: list[str], index: int, limit: int = 4) -> list[str]:
    result: list[str] = []
    cursor = index - 1
    while cursor >= 0 and len(result) < limit:
        value = lines[cursor].strip()
        if not value:
            if result:
                break
            cursor -= 1
            continue
        if not value.startswith(";"):
            break
        text = value.lstrip(";").strip()
        if text:
            result.append(text)
        cursor -= 1
    result.reverse()
    return result


def instruction_signature(lines: list[str], index: int, limit: int = 8) -> list[str]:
    result: list[str] = []
    for cursor in range(index + 1, min(len(lines), index + 40)):
        code = strip_comment(lines[cursor]).strip()
        label = LABEL_RE.match(code)
        if label and not label.group(1).startswith("@"):
            break
        match = INSTRUCTION_RE.match(code)
        if match:
            result.append(match.group(1).lower())
            if len(result) >= limit:
                break
    return result


def edge_counts(control_flow_path: Path | None) -> tuple[dict[str, int], dict[str, int]]:
    incoming: dict[str, int] = {}
    outgoing: dict[str, int] = {}
    if control_flow_path is None:
        return incoming, outgoing
    raw = json.loads(control_flow_path.read_text(encoding="utf-8"))
    groups = [raw.get("direct_edges", {}), raw.get("address_table_references", {})]
    items: list[dict[str, object]] = []
    for group in groups:
        items.extend(group.get("all_items", []))
        items.extend(group.get("unresolved_items", []))
        items.extend(group.get("local_scope_aliases", []))
    # Older report schemas do not retain all resolved edges. Counts remain optional.
    for edge in items:
        source = str(edge.get("source", ""))
        target = str(edge.get("resolved_as") or edge.get("target") or "")
        if source:
            outgoing[source.split("::", 1)[0]] = outgoing.get(source.split("::", 1)[0], 0) + 1
        if target:
            incoming[target.split("::", 1)[0]] = incoming.get(target.split("::", 1)[0], 0) + 1
    return incoming, outgoing


def build_report(labels_path: Path, sources: list[Path], revision: str,
                 control_flow_path: Path | None = None) -> dict[str, object]:
    labels = load_labels(labels_path)
    definitions, contents = source_definitions(sources)
    incoming, outgoing = edge_counts(control_flow_path)
    items: list[dict[str, object]] = []
    for name in sorted(name for name in labels if GENERIC_RE.match(name)):
        occurrences = definitions.get(name, [])
        contexts: list[dict[str, object]] = []
        for occurrence in occurrences:
            file_name = str(occurrence["file"])
            index = int(occurrence["index"])
            lines = contents[file_name]
            previous, following = nearest_global_labels(lines, index)
            contexts.append(
                {
                    "file": file_name,
                    "line": int(occurrence["line"]),
                    "previous_global_label": previous,
                    "next_global_label": following,
                    "preceding_comments": preceding_comments(lines, index),
                    "instruction_signature": instruction_signature(lines, index),
                }
            )
        if name.startswith("__") or name in {"size", "bank", ".size", ".bank"}:
            classification = "linker_generated_symbol"
        elif occurrences:
            classification = "source_label_pending_semantic_audit"
        else:
            classification = "generated_or_external_symbol"
        items.append(
            {
                "name": name,
                "addresses": [f"0x{address:04X}" for address in labels[name]],
                "classification": classification,
                "source_occurrences": contexts,
                "incoming_metadata_edges": incoming.get(name, 0),
                "outgoing_metadata_edges": outgoing.get(name, 0),
            }
        )
    classified_generated = sum(
        1 for item in items if item["classification"] != "source_label_pending_semantic_audit"
    )
    pending = sum(
        1 for item in items if item["classification"] == "source_label_pending_semantic_audit"
    )
    return {
        "schema": 1,
        "source_revision": revision,
        "generic_labels_total": len(items),
        "generated_symbols_classified": classified_generated,
        "source_labels_pending_semantic_audit": pending,
        "items": items,
        "interpretation": (
            "Generated/linker symbols are classified by role. Source labels remain pending "
            "until a human-readable functional name and evidence are recorded."
        ),
    }


def self_test() -> int:
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        labels = root / "sample.lbl"
        source = root / "sample.asm"
        labels.write_text("al 008000 .L8000\nal 000000 .__RAM_START__\n", encoding="utf-8")
        source.write_text("; test branch\nL8000:\n lda #0\n rts\n", encoding="utf-8")
        report = build_report(labels, [source], "self-test")
        if report["generic_labels_total"] != 2 or report["source_labels_pending_semantic_audit"] != 1:
            print(json.dumps(report, indent=2), file=sys.stderr)
            return 1
    print("prg_symbol_context self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("labels", type=Path, nargs="?")
    parser.add_argument("sources", type=Path, nargs="*")
    parser.add_argument("--control-flow", type=Path)
    parser.add_argument("--source-revision", default="unknown")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.labels is None or not args.sources:
        parser.error("labels and source files are required unless --self-test is used")
    try:
        report = build_report(
            args.labels, args.sources, args.source_revision, args.control_flow
        )
    except (OSError, ValueError, TypeError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "PRG generic-symbol context: "
        f"total={report['generic_labels_total']} "
        f"generated={report['generated_symbols_classified']} "
        f"pending_source={report['source_labels_pending_semantic_audit']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
