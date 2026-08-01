#!/usr/bin/env python3
"""Recursive 6502 disassembler for the user-supplied Tetris NES PRG.

The script reads a legal ROM at runtime and writes a bank-aware assembly listing.
It does not embed or redistribute ROM bytes in this repository. The output is an
analysis artifact, not yet a source file guaranteed to reassemble identically.
"""

from __future__ import annotations

import argparse
from collections import defaultdict, deque
from dataclasses import dataclass
import json
from pathlib import Path
import sys
from typing import Iterable

# Official NMOS 6502 instructions used by commercial NES software.
# opcode: (mnemonic, addressing mode)
OPS: dict[int, tuple[str, str]] = {
    0x00:("BRK","imp"),0x01:("ORA","indx"),0x05:("ORA","zp"),0x06:("ASL","zp"),
    0x08:("PHP","imp"),0x09:("ORA","imm"),0x0A:("ASL","acc"),0x0D:("ORA","abs"),0x0E:("ASL","abs"),
    0x10:("BPL","rel"),0x11:("ORA","indy"),0x15:("ORA","zpx"),0x16:("ASL","zpx"),
    0x18:("CLC","imp"),0x19:("ORA","absy"),0x1D:("ORA","absx"),0x1E:("ASL","absx"),
    0x20:("JSR","abs"),0x21:("AND","indx"),0x24:("BIT","zp"),0x25:("AND","zp"),0x26:("ROL","zp"),
    0x28:("PLP","imp"),0x29:("AND","imm"),0x2A:("ROL","acc"),0x2C:("BIT","abs"),0x2D:("AND","abs"),0x2E:("ROL","abs"),
    0x30:("BMI","rel"),0x31:("AND","indy"),0x35:("AND","zpx"),0x36:("ROL","zpx"),
    0x38:("SEC","imp"),0x39:("AND","absy"),0x3D:("AND","absx"),0x3E:("ROL","absx"),
    0x40:("RTI","imp"),0x41:("EOR","indx"),0x45:("EOR","zp"),0x46:("LSR","zp"),
    0x48:("PHA","imp"),0x49:("EOR","imm"),0x4A:("LSR","acc"),0x4C:("JMP","abs"),0x4D:("EOR","abs"),0x4E:("LSR","abs"),
    0x50:("BVC","rel"),0x51:("EOR","indy"),0x55:("EOR","zpx"),0x56:("LSR","zpx"),
    0x58:("CLI","imp"),0x59:("EOR","absy"),0x5D:("EOR","absx"),0x5E:("LSR","absx"),
    0x60:("RTS","imp"),0x61:("ADC","indx"),0x65:("ADC","zp"),0x66:("ROR","zp"),
    0x68:("PLA","imp"),0x69:("ADC","imm"),0x6A:("ROR","acc"),0x6C:("JMP","ind"),0x6D:("ADC","abs"),0x6E:("ROR","abs"),
    0x70:("BVS","rel"),0x71:("ADC","indy"),0x75:("ADC","zpx"),0x76:("ROR","zpx"),
    0x78:("SEI","imp"),0x79:("ADC","absy"),0x7D:("ADC","absx"),0x7E:("ROR","absx"),
    0x81:("STA","indx"),0x84:("STY","zp"),0x85:("STA","zp"),0x86:("STX","zp"),
    0x88:("DEY","imp"),0x8A:("TXA","imp"),0x8C:("STY","abs"),0x8D:("STA","abs"),0x8E:("STX","abs"),
    0x90:("BCC","rel"),0x91:("STA","indy"),0x94:("STY","zpx"),0x95:("STA","zpx"),0x96:("STX","zpy"),
    0x98:("TYA","imp"),0x99:("STA","absy"),0x9A:("TXS","imp"),0x9D:("STA","absx"),
    0xA0:("LDY","imm"),0xA1:("LDA","indx"),0xA2:("LDX","imm"),0xA4:("LDY","zp"),0xA5:("LDA","zp"),0xA6:("LDX","zp"),
    0xA8:("TAY","imp"),0xA9:("LDA","imm"),0xAA:("TAX","imp"),0xAC:("LDY","abs"),0xAD:("LDA","abs"),0xAE:("LDX","abs"),
    0xB0:("BCS","rel"),0xB1:("LDA","indy"),0xB4:("LDY","zpx"),0xB5:("LDA","zpx"),0xB6:("LDX","zpy"),
    0xB8:("CLV","imp"),0xB9:("LDA","absy"),0xBA:("TSX","imp"),0xBC:("LDY","absx"),0xBD:("LDA","absx"),0xBE:("LDX","absy"),
    0xC0:("CPY","imm"),0xC1:("CMP","indx"),0xC4:("CPY","zp"),0xC5:("CMP","zp"),0xC6:("DEC","zp"),
    0xC8:("INY","imp"),0xC9:("CMP","imm"),0xCA:("DEX","imp"),0xCC:("CPY","abs"),0xCD:("CMP","abs"),0xCE:("DEC","abs"),
    0xD0:("BNE","rel"),0xD1:("CMP","indy"),0xD5:("CMP","zpx"),0xD6:("DEC","zpx"),
    0xD8:("CLD","imp"),0xD9:("CMP","absy"),0xDD:("CMP","absx"),0xDE:("DEC","absx"),
    0xE0:("CPX","imm"),0xE1:("SBC","indx"),0xE4:("CPX","zp"),0xE5:("SBC","zp"),0xE6:("INC","zp"),
    0xE8:("INX","imp"),0xE9:("SBC","imm"),0xEA:("NOP","imp"),0xEC:("CPX","abs"),0xED:("SBC","abs"),0xEE:("INC","abs"),
    0xF0:("BEQ","rel"),0xF1:("SBC","indy"),0xF5:("SBC","zpx"),0xF6:("INC","zpx"),
    0xF8:("SED","imp"),0xF9:("SBC","absy"),0xFD:("SBC","absx"),0xFE:("INC","absx"),
}

MODE_LENGTH = {"imp":1,"acc":1,"imm":2,"zp":2,"zpx":2,"zpy":2,
               "indx":2,"indy":2,"rel":2,"abs":3,"absx":3,"absy":3,"ind":3}
BRANCHES = {"BPL","BMI","BVC","BVS","BCC","BCS","BNE","BEQ"}
TERMINATORS = {"BRK","RTI","RTS"}

@dataclass(frozen=True)
class Instruction:
    address: int
    opcode: int
    mnemonic: str
    mode: str
    operand: int | None
    length: int


def parse_rom(path: Path) -> tuple[bytes, bytes]:
    data = path.read_bytes()
    if len(data) < 16 or data[:4] != b"NES\x1a":
        raise ValueError("not an iNES/NES 2.0 ROM")
    trainer = 512 if data[6] & 0x04 else 0
    prg_size = data[4] * 16384
    if (data[7] & 0x0C) == 0x08 and (data[9] & 0x0F) != 0x0F:
        prg_size = (((data[9] & 0x0F) << 8) | data[4]) * 16384
    start = 16 + trainer
    end = start + prg_size
    if end > len(data):
        raise ValueError("PRG size exceeds file length")
    if prg_size != 32768:
        raise ValueError(f"expected 32768-byte PRG, found {prg_size}")
    return data, data[start:end]


def cpu_to_offset(address: int) -> int | None:
    if 0x8000 <= address < 0xC000:
        return address - 0x8000
    if 0xC000 <= address <= 0xFFFF:
        return 0x4000 + address - 0xC000
    return None


def offset_to_cpu(offset: int) -> int:
    return 0x8000 + offset if offset < 0x4000 else 0xC000 + offset - 0x4000


def read_word(prg: bytes, address: int) -> int | None:
    offset = cpu_to_offset(address)
    if offset is None or offset + 1 >= len(prg):
        return None
    return prg[offset] | (prg[offset + 1] << 8)


def load_symbols(path: Path) -> tuple[dict[int, str], list[tuple[int, int, str]]]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    symbols = {int(key, 0): str(value) for key, value in raw.get("symbols", {}).items()}
    ranges = []
    for item in raw.get("data_ranges", []):
        start = int(item["start"], 0)
        length = int(item["length"])
        ranges.append((start, start + length, str(item.get("name", "data"))))
    return symbols, ranges


def in_data_range(address: int, data_ranges: Iterable[tuple[int, int, str]]) -> bool:
    return any(start <= address < end for start, end, _ in data_ranges)


def decode(prg: bytes, address: int) -> Instruction | None:
    offset = cpu_to_offset(address)
    if offset is None or offset >= len(prg):
        return None
    opcode = prg[offset]
    spec = OPS.get(opcode)
    if spec is None:
        return None
    mnemonic, mode = spec
    length = MODE_LENGTH[mode]
    if offset + length > len(prg):
        return None
    operand: int | None = None
    if length == 2:
        operand = prg[offset + 1]
        if mode == "rel":
            displacement = operand if operand < 0x80 else operand - 0x100
            operand = (address + 2 + displacement) & 0xFFFF
    elif length == 3:
        operand = prg[offset + 1] | (prg[offset + 2] << 8)
    return Instruction(address, opcode, mnemonic, mode, operand, length)


def recursive_disassemble(prg: bytes, roots: Iterable[int],
                          data_ranges: list[tuple[int, int, str]]) -> tuple[dict[int, Instruction], dict[int, set[int]], set[int]]:
    queue = deque(roots)
    instructions: dict[int, Instruction] = {}
    xrefs: dict[int, set[int]] = defaultdict(set)
    targets: set[int] = set(roots)
    occupied: set[int] = set()

    while queue:
        address = queue.popleft()
        while cpu_to_offset(address) is not None:
            if address in instructions or in_data_range(address, data_ranges):
                break
            instruction = decode(prg, address)
            if instruction is None:
                break
            byte_addresses = {(address + index) & 0xFFFF for index in range(instruction.length)}
            if occupied.intersection(byte_addresses):
                break
            instructions[address] = instruction
            occupied.update(byte_addresses)
            next_address = (address + instruction.length) & 0xFFFF

            if instruction.mnemonic in BRANCHES and instruction.operand is not None:
                target = instruction.operand
                if cpu_to_offset(target) is not None and not in_data_range(target, data_ranges):
                    targets.add(target)
                    xrefs[target].add(address)
                    queue.append(target)
                address = next_address
                continue
            if instruction.mnemonic == "JSR" and instruction.operand is not None:
                target = instruction.operand
                if cpu_to_offset(target) is not None and not in_data_range(target, data_ranges):
                    targets.add(target)
                    xrefs[target].add(address)
                    queue.append(target)
                address = next_address
                continue
            if instruction.mnemonic == "JMP":
                if instruction.mode == "abs" and instruction.operand is not None:
                    target = instruction.operand
                    if cpu_to_offset(target) is not None and not in_data_range(target, data_ranges):
                        targets.add(target)
                        xrefs[target].add(address)
                        queue.append(target)
                break
            if instruction.mnemonic in TERMINATORS:
                break
            address = next_address
    return instructions, xrefs, targets


def operand_text(instruction: Instruction, labels: dict[int, str]) -> str:
    value = instruction.operand
    mode = instruction.mode
    if mode == "imp": return ""
    if mode == "acc": return "A"
    if value is None: return ""
    if mode == "imm": return f"#${value:02X}"
    if mode == "zp": return f"${value:02X}"
    if mode == "zpx": return f"${value:02X},X"
    if mode == "zpy": return f"${value:02X},Y"
    if mode == "indx": return f"(${value:02X},X)"
    if mode == "indy": return f"(${value:02X}),Y"
    if mode == "ind": return f"(${value:04X})"
    if mode == "rel": return labels.get(value, f"L{value:04X}")
    base = labels.get(value, f"${value:04X}")
    if mode == "absx": return f"{base},X"
    if mode == "absy": return f"{base},Y"
    return base


def emit_listing(prg: bytes, instructions: dict[int, Instruction],
                 xrefs: dict[int, set[int]], labels: dict[int, str],
                 data_ranges: list[tuple[int, int, str]]) -> str:
    lines = [
        "; Generated from a user-supplied ROM by tools/disassemble_prg.py",
        "; Analysis listing only; no ROM bytes are stored in the repository.",
        ".setcpu \"6502\"",
        "",
    ]
    instruction_bytes = {address + index
                         for address, ins in instructions.items()
                         for index in range(ins.length)}
    range_names = {start: name for start, _, name in data_ranges}

    for bank, start_offset, end_offset, origin in (
        (0, 0x0000, 0x4000, 0x8000),
        (1, 0x4000, 0x8000, 0xC000),
    ):
        lines.extend([f'.segment "PRG{bank}"', f".org ${origin:04X}", ""])
        offset = start_offset
        while offset < end_offset:
            address = offset_to_cpu(offset)
            if address in labels:
                comment = ""
                if address in xrefs:
                    refs = ", ".join(f"${source:04X}" for source in sorted(xrefs[address]))
                    comment = f" ; referenced from {refs}"
                lines.append(f"{labels[address]}:{comment}")
            if address in range_names and address not in labels:
                lines.append(f"{range_names[address]}:")
            instruction = instructions.get(address)
            if instruction:
                raw = prg[offset:offset + instruction.length]
                raw_text = " ".join(f"{byte:02X}" for byte in raw)
                operand = operand_text(instruction, labels)
                assembly = f"{instruction.mnemonic} {operand}".rstrip()
                lines.append(f"    {assembly:<18} ; ${address:04X}  {raw_text}")
                offset += instruction.length
                continue

            chunk = []
            chunk_start = offset
            while offset < end_offset and len(chunk) < 16:
                current = offset_to_cpu(offset)
                if current in instructions or (current in labels and offset != chunk_start):
                    break
                if current in range_names and offset != chunk_start:
                    break
                if current in instruction_bytes:
                    break
                chunk.append(prg[offset])
                offset += 1
            if not chunk:
                # An operand byte reached through overlapping or malformed control flow.
                chunk.append(prg[offset])
                offset += 1
            bytes_text = ", ".join(f"${byte:02X}" for byte in chunk)
            lines.append(f"    .byte {bytes_text:<76} ; ${offset_to_cpu(chunk_start):04X}")
        lines.append("")
    return "\n".join(lines)



def aggressive_roots(prg: bytes, data_ranges: list[tuple[int, int, str]]) -> set[int]:
    roots: set[int] = set()
    for offset in range(len(prg) - 2):
        if prg[offset] not in (0x20, 0x4C):
            continue
        target = prg[offset + 1] | (prg[offset + 2] << 8)
        if (cpu_to_offset(target) is not None and
                not in_data_range(target, data_ranges) and
                decode(prg, target) is not None):
            roots.add(target)
    return roots


def write_callgraph(path: Path, instructions: dict[int, Instruction],
                    labels: dict[int, str]) -> None:
    edges: set[tuple[int, int]] = set()
    for address, instruction in instructions.items():
        if instruction.mnemonic not in {"JSR", "JMP"} or instruction.mode != "abs":
            continue
        if instruction.operand is None or instruction.operand not in labels:
            continue
        edges.add((address, instruction.operand))
    lines = ["digraph tetris_prg {", "  rankdir=LR;"]
    nodes = {node for edge in edges for node in edge}
    for node in sorted(nodes):
        label = labels.get(node, f"L{node:04X}")
        lines.append(f'  n{node:04X} [label="{label}\n${node:04X}"];')
    for source, target in sorted(edges):
        lines.append(f'  n{source:04X} -> n{target:04X};')
    lines.append("}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_report(path: Path, vector_roots: list[int], roots: list[int],
                 instructions: dict[int, Instruction], labels: dict[int, str],
                 prg_size: int, aggressive: bool) -> None:
    code_bytes = sum(instruction.length for instruction in instructions.values())
    report = {
        "vector_roots": [f"0x{root:04X}" for root in vector_roots],
        "root_count": len(roots),
        "aggressive_root_count": max(0, len(roots) - len(vector_roots)),
        "aggressive": aggressive,
        "instructions": len(instructions),
        "code_bytes": code_bytes,
        "prg_bytes": prg_size,
        "recursive_coverage_percent": round(code_bytes * 100.0 / prg_size, 4),
        "labels": len(labels),
        "warning": "Recursive coverage is heuristic and is not verified decompilation progress."
    }
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

def self_test() -> int:
    required = {0x00,0x20,0x4C,0x60,0x6C,0xA9,0xD0,0xEA,0xFE}
    if not required.issubset(OPS):
        print("opcode table self-test failed", file=sys.stderr)
        return 1
    if any(mode not in MODE_LENGTH for _, mode in OPS.values()):
        print("unknown addressing mode", file=sys.stderr)
        return 1
    if len(OPS) != 151:
        print(f"expected 151 official opcodes, found {len(OPS)}", file=sys.stderr)
        return 1
    print("6502 opcode table self-test passed")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path, nargs="?")
    parser.add_argument("-o", "--output", type=Path)
    parser.add_argument("--symbols", type=Path,
                        default=Path(__file__).with_name("tetris_symbols.json"))
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--aggressive", action="store_true",
                        help="also seed recursion from plausible JSR/JMP operands; may include false positives")
    parser.add_argument("--dot", type=Path, help="write a Graphviz call graph")
    parser.add_argument("--report", type=Path, help="write a machine-readable JSON report")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.rom is None:
        parser.error("ROM path is required unless --self-test is used")

    try:
        _, prg = parse_rom(args.rom)
        symbols, data_ranges = load_symbols(args.symbols)
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    vectors = {
        "nmi_entry": read_word(prg, 0xFFFA),
        "reset_entry": read_word(prg, 0xFFFC),
        "irq_entry": read_word(prg, 0xFFFE),
    }
    vector_roots = [address for address in vectors.values() if address is not None]
    roots = list(vector_roots)
    if args.aggressive:
        roots.extend(sorted(aggressive_roots(prg, data_ranges)))
    roots = sorted(set(roots))
    for name, address in vectors.items():
        if address is not None:
            symbols.setdefault(address, name)

    instructions, xrefs, targets = recursive_disassemble(prg, roots, data_ranges)
    for target in targets:
        symbols.setdefault(target, f"L{target:04X}")
    listing = emit_listing(prg, instructions, xrefs, symbols, data_ranges)
    output = args.output or args.rom.with_suffix(".recursive.asm")
    output.write_text(listing, encoding="utf-8")
    if args.dot:
        write_callgraph(args.dot, instructions, symbols)
    if args.report:
        write_report(args.report, vector_roots, roots, instructions, symbols,
                     len(prg), args.aggressive)

    code_bytes = sum(instruction.length for instruction in instructions.values())
    print(f"output: {output}")
    print(f"vector_roots: {', '.join(f'${root:04X}' for root in vector_roots)}")
    print(f"root_count: {len(roots)}")
    print(f"instructions: {len(instructions)}")
    print(f"code_bytes: {code_bytes} / {len(prg)} ({code_bytes * 100.0 / len(prg):.2f}%)")
    print(f"labels: {len(symbols)}")
    print(f"mode: {'aggressive heuristic' if args.aggressive else 'vector-rooted conservative'}")
    print("note: recursive coverage is not the same as verified decompilation progress")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
