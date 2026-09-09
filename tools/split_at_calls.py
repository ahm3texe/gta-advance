#!/usr/bin/env python3
"""Keep the old linear BL-target experiment for forensic reference.

The walker in audit_boundaries.py treats an unconditional `b` as intra-function
flow. But GCC also uses `b` for a TAIL CALL: a function finishes its work and
jumps to its neighbor. That is why consecutive functions were merged into one
record in Phase 0.

That assumption is WRONG for this ROM: literal pools can look like a BL, and the
game can enter shared intra-function blocks with a BL. In the 2026-09-04 review
all 52 splits this tool produced were reverted. Write mode is permanently
disabled.

Usage:
    python3 tools/split_at_calls.py                 # explain why it is disabled
    python3 tools/split_at_calls.py --unsafe-report # the historical raw report
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000


def call_targets(rom: bytes, rows: list[tuple[int, int]]) -> set[int]:
    targets = set()
    for address, size in rows:
        base = address - ROM_BASE
        for i in range(0, max(0, size - 2), 2):
            hw1 = int.from_bytes(rom[base + i:base + i + 2], "little")
            hw2 = int.from_bytes(rom[base + i + 2:base + i + 4], "little")
            if (hw1 & 0xF800) == 0xF000 and (hw2 & 0xF800) == 0xF800:
                offset = hw1 & 0x7FF
                if offset & 0x400:
                    offset -= 0x800
                targets.add(address + i + 4 + (offset << 12) + ((hw2 & 0x7FF) << 1))
    return targets


def main() -> None:
    apply = "--apply" in sys.argv
    if apply:
        sys.exit("STOPPED: --apply is permanently disabled; the linear BL scan "
                 "produced 52 false boundaries")
    if "--unsafe-report" not in sys.argv:
        print("DISABLED: the linear BL scan can mistake literal/shared blocks "
              "for functions. Details: docs/WORKLOG.md (2026-09-04).")
        return
    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fields = list(rows[0].keys())

    sized = [(int(r["address"], 16), int(r["size"] or 0)) for r in rows
             if r["size"].strip()]
    targets = call_targets(rom, sized)
    starts = {a for a, _ in sized}

    # For each record, call targets falling inside its body = split points
    splits: dict[int, list[int]] = {}
    for address, size in sized:
        inner = sorted(t for t in targets
                       if address < t < address + size and t not in starts)
        if inner:
            splits[address] = inner

    total_new = sum(len(v) for v in splits.values())
    print(f"{len(splits)} records would be split, {total_new} new function boundaries\n")
    print(f"{'record':12} {'size':>6}  split points")
    print("-" * 60)
    for address in sorted(splits)[:15]:
        points = " ".join(f"0x{t:08X}" for t in splits[address][:4])
        size = dict(sized)[address]
        more = "..." if len(splits[address]) > 4 else ""
        print(f"0x{address:08X} {size:>6}  {points}{more}")

    if not apply:
        print("\n(report only; use --apply to split)")
        return

    out = []
    added: set[int] = set()      # the same call target can sit inside two
                                 # parents; add the child only ONCE
    for row in rows:
        address = int(row["address"], 16)
        if address not in splits:
            out.append(row)
            continue
        size = int(row["size"])
        points = splits[address] + [address + size]
        # The first fragment keeps the old record's name and shrinks.
        row["size"] = str(points[0] - address)
        note = row["notes"].strip('"')
        row["notes"] = (note + "; had been merged by a tail call, "
                        "split by split_at_calls.py").lstrip("; ")
        out.append(row)
        for i, start in enumerate(splits[address]):
            if start in added:
                continue
            added.add(start)
            out.append({
                "address": f"0x{start:08X}", "name": f"FUN_{start:08x}",
                "size": str(points[i + 1] - start), "status": "discovered",
                "module": row["module"],
                "notes": f"0x{address:08X} kaydinin icinde saklaniyordu; "
                         f"a separate function because it is a call target",
            })
    out.sort(key=lambda r: int(r["address"], 16))
    with FUNCTIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(out)
    print(f"\nfunctions.csv: {len(splits)} records split, "
          f"{total_new} new functions.")


if __name__ == "__main__":
    main()
