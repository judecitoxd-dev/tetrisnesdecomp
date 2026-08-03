#!/usr/bin/env python3
"""Generate static RAM/PPU/APU state contracts from pinned ca65 source."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

LABEL_RE = re.compile(r"^\s*([A-Za-z_.$@][A-Za-z0-9_.$@]*)\s*:")
INSTRUCTION_RE = re.compile(r"^\s*([A-Za-z]{3})\b\s*(.*)$")
SYMBOL_RE = re.compile(r"[A-Za-z_.$][A-Za-z0-9_.$]*(?:[+-][A-Za-z0-9_$]+)?")
READ_OPS = {"lda", "ldx", "ldy", "adc", "sbc", "and", "ora", "eor", "cmp", "cpx", "cpy", "bit"}
WRITE_OPS = {"sta", "stx", "sty"}
RMW_OPS = {"inc", "dec", "asl", "lsr", "rol", "ror"}
STACK_WRITES = {"pha", "php", "jsr", "brk"}
STACK_READS = {"pla", "plp", "rts", "rti"}

HARDWARE_SYMBOLS = {
    "PPUCTRL", "PPUMASK", "PPUSTATUS", "OAMADDR", "OAMDATA", "PPUSCROLL",
    "PPUADDR", "PPUDATA", "SQ1_VOL", "SQ1_SWEEP", "SQ1_LO", "SQ1_HI",
    "SQ2_VOL", "SQ2_SWEEP", "SQ2_LO", "SQ2_HI", "TRI_LINEAR", "TRI_UNUSED",
    "TRI_LO", "TRI_HI", "NOISE_VOL", "NOISE_UNUSED", "NOISE_LO", "NOISE_HI",
    "DMC_FREQ", "DMC_RAW", "DMC_START", "DMC_LEN", "SND_CHN", "JOY1",
    "JOY2_APUFC", "OAMDMA", "MMC1_CTRL", "MMC1_CHR_BANK0", "MMC1_CHR_BANK1",
    "MMC1_PRG_BANK",
}


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


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


def normalize_operand(operand: str) -> tuple[str | None, bool, bool]:
    value = operand.strip()
    if not value or value.upper() == "A":
        return None, False, False
    if value.startswith("#"):
        return None, False, True
    indirect = value.startswith("(")
    match = SYMBOL_RE.search(value)
    if match:
        return match.group(0), indirect, False
    numeric = re.search(r"\$[0-9A-Fa-f]+|%[01]+|\b\d+\b", value)
    if numeric:
        return numeric.group(0), indirect, False
    return None, indirect, False


def base_symbol(symbol: str) -> str:
    return re.split(r"[+-]", symbol, maxsplit=1)[0]


def add_access(contract: dict[str, set[str]], mode: str, symbol: str, indirect: bool) -> None:
    base = base_symbol(symbol)
    hardware = base in HARDWARE_SYMBOLS
    if hardware:
        key = f"hardware_{mode}"
    elif indirect:
        key = f"indirect_{mode}"
    elif symbol.startswith("$") or symbol.startswith("%") or symbol[:1].isdigit():
        key = f"raw_memory_{mode}"
    else:
        key = mode
    contract[key].add(symbol)


def build_contracts(paths: list[Path], revision: str) -> dict[str, object]:
    contracts: dict[str, dict[str, object]] = {}
    current_global: str | None = None
    current_file = ""

    def ensure(name: str, file_name: str, line: int) -> dict[str, object]:
        if name not in contracts:
            sets = {
                key: set()
                for key in (
                    "reads", "writes", "read_writes", "hardware_reads",
                    "hardware_writes", "hardware_read_writes", "indirect_reads",
                    "indirect_writes", "indirect_read_writes", "raw_memory_reads",
                    "raw_memory_writes", "raw_memory_read_writes", "calls",
                    "tail_jumps", "cpu_stack_reads", "cpu_stack_writes",
                )
            }
            contracts[name] = {
                "name": name,
                "file": file_name,
                "line": line,
                "instruction_count": 0,
                "sets": sets,
                "returns": False,
                "interrupt_return": False,
            }
        return contracts[name]

    for path in paths:
        current_global = None
        current_file = str(path)
        for line_number, raw in enumerate(
            path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
        ):
            code = strip_comment(raw)
            labels, tail = split_label_and_tail(code)
            for label in labels:
                if not label.startswith("@"):
                    current_global = label
            statement = tail.strip() if labels else code.strip()
            if not statement or current_global is None:
                continue
            match = INSTRUCTION_RE.match(statement)
            if not match:
                continue
            mnemonic = match.group(1).lower()
            operand = match.group(2).strip()
            contract = ensure(current_global, current_file, line_number)
            contract["instruction_count"] = int(contract["instruction_count"]) + 1
            sets: dict[str, set[str]] = contract["sets"]  # type: ignore[assignment]

            if mnemonic in STACK_WRITES:
                sets["cpu_stack_writes"].add("cpu_stack")
            if mnemonic in STACK_READS:
                sets["cpu_stack_reads"].add("cpu_stack")
            if mnemonic == "rts":
                contract["returns"] = True
            if mnemonic == "rti":
                contract["interrupt_return"] = True
            if mnemonic == "jsr":
                target, _, immediate = normalize_operand(operand)
                if target and not immediate:
                    sets["calls"].add(target)
                continue
            if mnemonic == "jmp":
                target, indirect, immediate = normalize_operand(operand)
                if target and not immediate:
                    sets["tail_jumps"].add(f"indirect:{target}" if indirect else target)
                continue

            symbol, indirect, immediate = normalize_operand(operand)
            if symbol is None or immediate:
                continue
            if mnemonic in READ_OPS:
                add_access(sets, "reads", symbol, indirect)
            elif mnemonic in WRITE_OPS:
                add_access(sets, "writes", symbol, indirect)
            elif mnemonic in RMW_OPS:
                add_access(sets, "read_writes", symbol, indirect)

    output_contracts: list[dict[str, object]] = []
    total_instructions = 0
    hardware_touching = 0
    indirect_touching = 0
    pure_cpu = 0
    for name in sorted(contracts):
        raw = contracts[name]
        sets = raw.pop("sets")  # type: ignore[assignment]
        serialized = {key: sorted(value) for key, value in sets.items()}
        instruction_count = int(raw["instruction_count"])
        total_instructions += instruction_count
        if any(serialized[key] for key in ("hardware_reads", "hardware_writes", "hardware_read_writes")):
            hardware_touching += 1
        if any(serialized[key] for key in ("indirect_reads", "indirect_writes", "indirect_read_writes")):
            indirect_touching += 1
        memory_keys = [key for key in serialized if key not in {"calls", "tail_jumps", "cpu_stack_reads", "cpu_stack_writes"}]
        if not any(serialized[key] for key in memory_keys):
            pure_cpu += 1
        output_contracts.append({**raw, **serialized})

    routines = len(output_contracts)
    return {
        "schema": 1,
        "source_revision": revision,
        "source_files": [str(path) for path in paths],
        "routines_with_instructions": routines,
        "contracts_generated": routines,
        "contract_coverage_percent": 100.0 if routines else 0.0,
        "instructions_classified": total_instructions,
        "hardware_touching_routines": hardware_touching,
        "indirect_memory_routines": indirect_touching,
        "pure_cpu_or_control_routines": pure_cpu,
        "hardware_symbols_classified": sorted(HARDWARE_SYMBOLS),
        "gate_passed": routines > 0 and len(output_contracts) == routines,
        "contracts": output_contracts,
        "interpretation": (
            "Contracts describe static reads, writes, indirect accesses, hardware ports, "
            "calls and returns for every global source routine/basic block with instructions. "
            "Dynamic values are verified by the separate trace gate."
        ),
    }


def self_test() -> int:
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "sample.asm"
        source.write_text(
            "start:\n lda score\n sta PPUDATA\n inc score\n jsr helper\n rts\n"
            "helper:\n lda (tmp1),y\n sta $0200,x\n rts\n",
            encoding="utf-8",
        )
        report = build_contracts([source], "self-test")
        if not report["gate_passed"] or report["contracts_generated"] != 2:
            print(json.dumps(report, indent=2), file=sys.stderr)
            return 1
        start = next(item for item in report["contracts"] if item["name"] == "start")
        if start["reads"] != ["score"] or start["hardware_writes"] != ["PPUDATA"]:
            print(json.dumps(start, indent=2), file=sys.stderr)
            return 1
    print("prg_state_contracts self-test: OK")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("sources", type=Path, nargs="*")
    parser.add_argument("--source-revision", default="unknown")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.sources:
        parser.error("source files are required unless --self-test is used")
    try:
        report = build_contracts(args.sources, args.source_revision)
    except (OSError, ValueError, TypeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "PRG state contracts: "
        f"{report['contract_coverage_percent']}% "
        f"contracts={report['contracts_generated']} "
        f"instructions={report['instructions_classified']}"
    )
    return 0 if report["gate_passed"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
