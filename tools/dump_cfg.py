#!/usr/bin/env python3
"""Extract a function's basic block graph.

Branch targets are followed RECURSIVELY; only addresses REACHABLE from the
entry are decoded as instructions, so a literal pool is never mistaken for
code (two tools made that same mistake in this session).

Output: for each block, its start/end, terminating instruction, successors and
how many predecessors it has. Blocks with more than one predecessor are marked
as a JOIN POINT -- in the source that usually means a shared tail or a label.

Usage: python3 tools/dump_cfg.py <address|name>
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_BASE = 0x08000000
CONDS = {0x0: "eq", 0x1: "ne", 0x2: "cs", 0x3: "cc", 0x4: "mi", 0x5: "pl",
         0x6: "vs", 0x7: "vc", 0x8: "hi", 0x9: "ls", 0xA: "ge", 0xB: "lt",
         0xC: "gt", 0xD: "le"}


def resolve(target: str) -> tuple:
    rows = list(csv.DictReader(
        (ROOT / "data/functions.csv").open(newline="", encoding="utf-8")))
    want = target.lower()
    for row in rows:
        if row["name"] == target or row["address"].lower() == want:
            return int(row["address"], 16), int(row["size"] or 0), row["name"]
    sys.exit(f"{target} is not in data/functions.csv")


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    addr, size, name = resolve(sys.argv[1])
    rom = (ROOT / "baserom.gba").read_bytes()

    def hw(a):
        o = a - ROM_BASE
        return int.from_bytes(rom[o:o + 2], "little")

    end = addr + size
    leaders = {addr}
    edges = {}          # block terminator -> (kind, successors)
    seen = set()
    pending = [addr]

    while pending:
        pc = pending.pop()
        while addr <= pc < end and pc not in seen:
            seen.add(pc)
            h = hw(pc)
            if (h & 0xF800) == 0xF000:              # 32-bit bl
                seen.add(pc + 2)
                pc += 4
                continue
            if (h & 0xF000) == 0xD000 and ((h >> 8) & 0xF) < 0xE:
                off = h & 0xFF
                off = off - 256 if off > 127 else off
                tgt = pc + 4 + off * 2
                nxt = pc + 2
                edges[pc] = (f"b{CONDS[(h >> 8) & 0xF]}", [tgt, nxt])
                leaders.update({tgt, nxt})
                pending.extend([tgt, nxt])
                break
            if (h & 0xF800) == 0xE000:              # unconditional branch
                off = h & 0x7FF
                off = off - 2048 if off > 1023 else off
                tgt = pc + 4 + off * 2
                edges[pc] = ("b", [tgt])
                leaders.add(tgt)
                pending.append(tgt)
                break
            if h == 0x4770 or (0xBD00 <= h <= 0xBDFF):
                edges[pc] = ("ret", [])
                break
            if (h & 0xFF87) == 0x4700:              # bx rN
                edges[pc] = ("ret", [])
                break
            pc += 2

    blocks = sorted(b for b in leaders if b in seen)
    preds = {b: 0 for b in blocks}
    for term, (_, succs) in edges.items():
        for s in succs:
            if s in preds:
                preds[s] += 1

    print(f"\n{name} @ 0x{addr:08X}  {size} bytes  {len(blocks)} blocks\n")
    for i, b in enumerate(blocks):
        stop = blocks[i + 1] if i + 1 < len(blocks) else end
        term = next((t for t in sorted(edges) if b <= t < stop), None)
        kind, succs = edges.get(term, ("(fall)", [stop]))
        tag = "  <- JOIN" if preds.get(b, 0) > 1 else ""
        arrow = " ".join(f"0x{s:08X}" for s in succs) or "(return)"
        print(f"  B{i:<2} 0x{b:08X}..0x{stop:08X}  {kind:<6} -> {arrow}"
              f"   onceller={preds.get(b, 0)}{tag}")


if __name__ == "__main__":
    main()
