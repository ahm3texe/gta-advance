#!/usr/bin/env python3
"""Find the DISPATCH CHAINS where rule 45 applies, directly in the ROM.

Why from the ROM
----------------
On the first attempt this scan was done over Ghidra's output by counting
"consecutive identical expression blocks". It gives FALSE POSITIVES: in
FUN_080108f4 it counted 139 repetitions, but that function does not contain a
repeated BRANCH BODY -- it contains a repeated GLOBAL ACCESS pattern (40
distinct DAT_ globals, nested loops). Textual similarity is the wrong
criterion.

The correct signature is on the ROM side: rule 45 applies only when there is a
long `cmp rX,#imm` chain against the SAME REGISTER -- that is, a long if/else
chain branching on a state variable. In FUN_080260a8 that chain had 12 links
and the branch bodies repeated; given a shared local, agbcc swallowed one
branch's block entirely (see docs/COMPILER.md rule 45).

This tool counts that chain directly from the machine code.

SCOPE LIMIT
-----------
Pool words can also happen to encode like a `cmp`; the chain is therefore
required to be against the same register with a plausible spacing between
links. Even so, the candidate list is not FILTERED, only WORTH REVIEWING.

Usage:
    python3 tools/scan_dispatch.py            # unmatched, >=512 bytes
    python3 tools/scan_dispatch.py --min 4 --size 256
"""
import argparse
import csv
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from verify_c_function import rom_bytes, ROM_BASE  # noqa: E402


def chains(buf):
    """Return consecutive `cmp rX,#imm` links against the same register."""
    hits = []
    for i in range(0, len(buf) - 1, 2):
        h = struct.unpack_from("<H", buf, i)[0]
        if (h >> 11) == 0x05:                      # 00101 rrr iiiiiiii
            hits.append((i, (h >> 8) & 7, h & 0xFF))
    best = []
    cur = []
    for hit in hits:
        if cur and hit[1] == cur[-1][1] and 0 < hit[0] - cur[-1][0] <= 400:
            cur.append(hit)
        else:
            if len(cur) > len(best):
                best = cur
            cur = [hit]
    if len(cur) > len(best):
        best = cur
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=int, default=5, help="minimum number of links in the chain")
    ap.add_argument("--size", type=int, default=512, help="minimum function size")
    ap.add_argument("--distinct", type=int, default=5,
                    help="minimum number of DISTINCT non-zero values in the chain")
    a = ap.parse_args()

    rom = rom_bytes()
    rows = []
    for r in csv.DictReader((ROOT / "data/functions.csv").open()):
        if r["status"] == "matching" or not r["size"]:
            continue
        size = int(r["size"])
        if size < a.size:
            continue
        addr = int(r["address"], 16)
        off = addr - ROM_BASE
        ch = chains(rom[off:off + size])
        vals = [c[2] for c in ch]
        # THE DISCRIMINATING CRITERION: the DISTINCT NON-ZERO values in the
        # chain. `cmp rX,#0` links are null checks, not dispatch; only
        # Non-zero and mutually distinct values indicate branching on a state
        # variable.
        distinct = len({v for v in vals if v})
        if len(ch) >= a.min and distinct >= a.distinct:
            rows.append((distinct, len(ch), size, r["name"], r["address"], vals))
    rows.sort(reverse=True)

    print(f"{'function':<20}{'address':<12}{'size':>6}{'distinct':>9}{'links':>7}  values")
    print("-" * 100)
    for distinct, n, size, name, addr, vals in rows:
        v = ", ".join(str(x) for x in vals[:12])
        if len(vals) > 12:
            v += ", ..."
        print(f"{name:<20}{addr:<12}{size:>6}{distinct:>7}{n:>7}  {v}")
    print(f"\n{len(rows)} candidates, {sum(r[2] for r in rows)} bytes in total")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
