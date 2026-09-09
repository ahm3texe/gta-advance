#!/usr/bin/env python3
"""Find pairs of functions that are STRUCTURALLY IDENTICAL.

Why
---
0x08031844 (472 bytes) consumed 341 thousand agent tokens and was closed as
"register allocation, no source-level lever". Yet its matching sibling
0x08031A1C was sitting in the ROM, and the ONLY difference between the two
bodies was the polarity of one comparison (`blt` <-> `bge`). Copying the
matching sibling's source and flipping that test gave a full match on the
first attempt (docs/WORKFLOW.md §10).

This tool turns that search from a manual step into a sweep.

HOW
---
Each function's body is split into 16-bit half-words and NORMALIZED:
position-dependent fields (branch distance, pool distance) are zeroed, while
the instruction's identity and its REGISTERS are preserved. If two functions
came from the same C, their normalized sequences are very close; the raw bytes
do not agree because their positions in the ROM differ.

This is a DIAGNOSTIC tool, not evidence. Nothing is recorded until the reported
pair is verified with `tools/diff_function.py`.

Usage:
    python3 tools/find_twins.py                # unmatched -> matching
    python3 tools/find_twins.py --unmatched    # unmatched -> unmatched (clusters)
    python3 tools/find_twins.py --min 0.90     # threshold (default 0.85)
"""
import argparse
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000


def normalize(body: bytes) -> tuple[int, ...]:
    """Make the half-words position-independent.

    Only ADDRESS-dependent fields are zeroed; register numbers and instruction
    identity are left untouched, because those are what discriminate.
    """
    out = []
    for i in range(0, len(body) - 1, 2):
        hw = body[i] | (body[i + 1] << 8)
        top5 = hw >> 11
        if top5 == 0b11100:            # B <imm11>
            hw &= 0xF800
        elif (hw >> 12) == 0b1101:     # B<cond> <imm8> - the condition is kept
            hw &= 0xFF00
        elif top5 in (0b11110, 0b11111):  # the BL pair
            hw &= 0xF800
        elif (hw >> 11) == 0b01001:    # LDR Rd,[PC,#imm8] - pool distance
            hw &= 0xF800 | 0x0700
        elif (hw >> 11) == 0b10101:    # ADD Rd,PC,#imm8
            hw &= 0xF800 | 0x0700
        out.append(hw)
    return tuple(out)


def similarity(a: tuple[int, ...], b: tuple[int, ...]) -> float:
    """The fraction of positions that are identical in two equal-length sequences."""
    n = min(len(a), len(b))
    if n == 0:
        return 0.0
    same = sum(1 for i in range(n) if a[i] == b[i])
    return same / max(len(a), len(b))


def load():
    rom = ROM.read_bytes()
    rows = list(csv.DictReader(FUNCTIONS.open(newline="", encoding="utf-8")))
    out = []
    for r in rows:
        size = int(r["size"] or 0)
        if size < 24:                  # similarity is meaningless for small stubs
            continue
        if "ARM" in r["notes"].upper().split():
            continue
        off = int(r["address"], 16) - ROM_BASE
        body = rom[off:off + size]
        if len(body) < size:
            continue
        out.append({
            "address": r["address"], "name": r["name"], "size": size,
            "status": r["status"], "norm": normalize(body),
        })
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=float, default=0.85)
    ap.add_argument("--unmatched", action="store_true",
                    help="compare unmatched against each other (finds clusters)")
    ap.add_argument("--limit", type=int, default=40)
    args = ap.parse_args()

    funcs = load()
    left = [f for f in funcs if f["status"] != "matching"]
    right = left if args.unmatched else [f for f in funcs if f["status"] == "matching"]

    # Bucket by length: only bodies within +-10% in length are compared.
    by_len: dict[int, list] = {}
    for f in right:
        by_len.setdefault(len(f["norm"]), []).append(f)
    lengths = sorted(by_len)

    hits = []
    for a in left:
        na = len(a["norm"])
        lo, hi = int(na * 0.9), int(na * 1.1) + 1
        best = None
        for n in lengths:
            if n < lo:
                continue
            if n > hi:
                break
            for b in by_len[n]:
                if b["address"] == a["address"]:
                    continue
                s = similarity(a["norm"], b["norm"])
                if best is None or s > best[0]:
                    best = (s, b)
        if best and best[0] >= args.min:
            hits.append((best[0], a, best[1]))

    hits.sort(key=lambda t: (-t[0] * t[1]["size"], -t[0]))
    kind = "unmatched" if args.unmatched else "MATCHING"
    print(f"{len(hits)} candidates ({args.min:.0%} and above, target: {kind} sibling)\n")
    print(f"{'similarity':>10} {'bytes':>6}  {'unmatched':<34} {'sibling':<34}")
    print("-" * 90)
    total = 0
    for s, a, b in hits[:args.limit]:
        total += a["size"]
        print(f"{s:>8.1%} {a['size']:>6}  {a['address']} {a['name'][:22]:<22} "
              f"{b['address']} {b['name'][:22]:<22}")
    print("-" * 90)
    print(f"total of the {min(len(hits), args.limit)} listed candidates: {total} bytes")
    print(f"total of all candidates: {sum(a['size'] for _, a, _ in hits)} bytes")


if __name__ == "__main__":
    main()
