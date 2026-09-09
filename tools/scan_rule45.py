#!/usr/bin/env python3
"""Detect functions where rule 45 applies, from Ghidra's output.

Rule 45 (docs/COMPILER.md): in long if/else chains, when several branches have
identical bodies agbcc merges them by cross-jumping and one branch's code
disappears entirely. If those branches are separate physical copies in the ROM,
then in the original source each branch had ITS OWN local variables.

This tool counts two signals:

  repeats  consecutive expression blocks in Ghidra's output that are identical
           apart from names. A high count means a high merging risk.
  locals   the number of distinct `local_XX` variables Ghidra produced. Ghidra
           shows these correctly; many locals indicate that the branches use
           separate locals.

Usage:
    python3 tools/scan_rule45.py                 # build/ghidra_out/*.c
    python3 tools/scan_rule45.py <directory>
"""
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def normalize(line):
    """Abstract away variable names so identically shaped blocks compare equal."""
    s = line.strip()
    s = re.sub(r"\blocal_[0-9a-f]+\b", "L", s)
    s = re.sub(r"\b[iup]?[A-Za-z]*Var\d+\b", "V", s)
    s = re.sub(r"\bDAT_[0-9a-f]+\b", "D", s)
    return s


def scan(path):
    lines = [normalize(l) for l in path.read_text(errors="replace").splitlines()]
    lines = [l for l in lines if l and not l.startswith(("/*", "*", "//"))]

    # Count repetitions of 6-line windows
    W = 6
    windows = Counter()
    for i in range(len(lines) - W):
        windows["\n".join(lines[i:i + W])] += 1
    repeats = sum(c - 1 for c in windows.values() if c > 1)

    text = path.read_text(errors="replace")
    locals_ = len(set(re.findall(r"\blocal_[0-9a-f]+\b", text)))
    chain = len(re.findall(r"\bif\s*\(|\belse if\s*\(", text))
    # Every warning saying Ghidra's output is INCOMPLETE. "Removing
    # unreachable block" ozellikle sinsi: cikti derli toplu gorunur ama
    # block(s) have been dropped, and source cannot be written from it.
    incomplete = any(w in text for w in (
        "Could not recover jumptable",
        "Removing unreachable block",
        "Bad instruction",
        "truncated",
    ))
    return repeats, locals_, chain, incomplete


def main():
    d = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build/ghidra_out"
    if not d.is_dir():
        sys.exit(f"{d} is missing. Run the batch export first (see docs/COMPILER.md).")

    rows = []
    for f in sorted(d.glob("*.c")):
        rep, loc, chain, jt = scan(f)
        rows.append((rep, loc, chain, jt, f.stem))
    rows.sort(reverse=True)

    print(f"{'function':<20}{'repeats':>8}{'locals':>7}{'chain':>6}  status")
    print("-" * 56)
    for rep, loc, chain, jt, name in rows:
        status = "Ghidra INCOMPLETE" if jt else ("RULE 45 CANDIDATE" if rep >= 3 and loc >= 6 else "-")
        print(f"{name:<20}{rep:>8}{loc:>7}{chain:>6}  {status}")
    n = sum(1 for r in rows if not r[3] and r[0] >= 3 and r[1] >= 6)
    print(f"\n{n} candidates (among those whose Ghidra output is complete)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
