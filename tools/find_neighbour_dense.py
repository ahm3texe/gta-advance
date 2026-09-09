#!/usr/bin/env python3
"""Select functions with matching neighbors whose C has NOT BEEN WRITTEN yet.

WHY
---
If the functions around one are already matching, that region's type vocabulary
(struct layouts, RAM symbols, call signatures) is ready; such candidates
typically close in one or two attempts. This selection used to be made by hand
(with one-off python snippets) and was NOT RECORDED IN THE REPOSITORY, so the
numbers in the plan could not be reproduced. This tool closes that gap.

This is a DIAGNOSTIC tool: a function is not easy merely because its
neighborhood is dense. Nothing is recorded until the resulting list is verified
with `tools/diff_function.py`.

CRITERIA (all adjustable)
------------------------------
  - status != matching
  - NOT ARM (the notes do not contain "ARM" as a separate word) -- ARM is
    closed because of agbcc_arm's 8-register ceiling (docs/STATUS.md)
  - there is NO definition with the SAME NAME under src/, i.e. not even a draft
    has been written (--include-drafts disables this filter and also lists near
    misses)
  - the size is within [--min-size, --max-size]
  - in address order, at least --neighbours matching functions lie in the
    9-wide window (i-4 .. i+4)

Usage:
    python3 tools/find_neighbour_dense.py                 # the default selection
    python3 tools/find_neighbour_dense.py --neighbours 4 --min-size 48 \
        --max-size 512 --include-drafts                   # the initial analysis's selection
    python3 tools/find_neighbour_dense.py --out data/neighbour_dense.csv
"""
import argparse
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS = ROOT / "data/functions.csv"

# Matches DEFINITIONS such as `int foo(...) {` / `static void *bar(...)\n{`;
# declarations (ending in `;`) are excluded.
DEFINITION = re.compile(r"^\w[\w\s\*]*?\b(\w+)\s*\([^;]*\)\s*\{", re.M)


def defined_in_sources() -> set[str]:
    """Names of functions whose body is written under src/."""
    names: set[str] = set()
    for path in sorted((ROOT / "src").glob("*/*.c")):
        text = path.read_text(encoding="utf-8", errors="ignore")
        names.update(DEFINITION.findall(text))
    return names


def is_arm(row: dict) -> bool:
    return "ARM" in row["notes"].upper().split()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--neighbours", type=int, default=3,
                    help="minimum matching functions in the 9-wide window (default 3)")
    ap.add_argument("--min-size", type=int, default=40)
    ap.add_argument("--max-size", type=int, default=520)
    ap.add_argument("--include-drafts", action="store_true",
                    help="also list those with a C draft (near misses)")
    ap.add_argument("--include-arm", action="store_true")
    ap.add_argument("--limit", type=int, default=30)
    ap.add_argument("--out", help="write the selection as CSV (leaves a record in the repo)")
    args = ap.parse_args()

    rows = list(csv.DictReader(FUNCTIONS.open(newline="", encoding="utf-8")))
    rows.sort(key=lambda r: int(r["address"], 16))
    drafted = set() if args.include_drafts else defined_in_sources()
    total = len(rows)

    picks = []
    for i, row in enumerate(rows):
        if row["status"] == "matching":
            continue
        if row["name"] in drafted:
            continue
        if not args.include_arm and is_arm(row):
            continue
        size = int(row["size"] or 0)
        if not (args.min_size <= size <= args.max_size):
            continue
        window = rows[max(0, i - 4):min(total, i + 5)]
        near = sum(1 for w in window if w["status"] == "matching")
        if near >= args.neighbours:
            picks.append((near, size, row))

    picks.sort(key=lambda t: (-t[0], -t[1]))
    bytes_total = sum(size for _, size, _ in picks)

    print(f"criteria: size {args.min_size}-{args.max_size}, neighbours >= {args.neighbours}, "
          f"drafts {'INCLUDED' if args.include_drafts else 'EXCLUDED'}, "
          f"ARM {'INCLUDED' if args.include_arm else 'EXCLUDED'}")
    print(f"{len(picks)} candidates / {bytes_total} bytes "
          f"({100 * bytes_total / 454258:.2f}% of ROM code)\n")
    print(f"{'address':<12} {'bytes':>5} {'neigh':>6}  name")
    print("-" * 62)
    for near, size, row in picks[:args.limit]:
        print(f"{row['address']:<12} {size:>5} {near:>6}  {row['name']}")
    if len(picks) > args.limit:
        print(f"... {len(picks) - args.limit} more candidates (raise --limit)")

    if args.out:
        out = ROOT / args.out
        with out.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle)
            writer.writerow(["address", "name", "size", "neighbours", "status"])
            for near, size, row in picks:
                writer.writerow([row["address"], row["name"], size, near, row["status"]])
        print(f"\n-> {args.out} ({len(picks)} rows)")


if __name__ == "__main__":
    main()
