#!/usr/bin/env python3
"""Compare every non-matching C source against the ROM and report CLOSENESS.

Why it exists
-------------
`make progress` only says matched/not matched; nowhere was it recorded which
function is one byte away and which is hundreds. That is exactly what is needed
to choose what comes next.

Measured values
---------------
  size       compiled size / size in the map
  difference bytes differing at the same offset + the absolute size difference
  closeness  1 - difference/mapped size (stays meaningful even when sizes differ)

CAUTION: the closeness percentage is a ROUGH ordering criterion, not a measure of
progress. A single register difference can shift the whole function and the
percentage drops abruptly; conversely a high percentage does not mean the last
byte will close easily. The real signal is how SMALL the difference COUNT is.

Usage:
    python3 tools/near_misses.py
    python3 tools/near_misses.py --csv build/near_misses.csv
"""
import argparse
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from agbcc_build import compile_and_link, DEFAULT_CC  # noqa: E402
from verify_c_function import rom_bytes, ROM_BASE      # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", help="also write the result to this file")
    a = ap.parse_args()

    rom = rom_bytes()
    fmap = {r["name"]: r for r in
            csv.DictReader((ROOT / "data/functions.csv").open())}

    todo = {}
    for r in csv.DictReader((ROOT / "data/c_sources.csv").open()):
        if r["matching"] != "yes":
            todo.setdefault(r["source"], []).append(r)

    rows = []
    for src, entries in sorted(todo.items()):
        path = ROOT / src
        try:
            blob, layout, _ = compile_and_link(path, DEFAULT_CC)
        except SystemExit as e:
            for e2 in entries:
                rows.append((None, e2["name"], e2["address"], 0, 0,
                             f"derlenmedi: {e}", src))
            continue
        for e in entries:
            name = e["name"]
            if name not in layout:
                rows.append((None, name, e["address"], 0, 0, "not in the link", src))
                continue
            off, n = layout[name]
            size = int(fmap[name]["size"] or 0)
            want = rom[int(e["address"], 16) - ROM_BASE:][:size]
            got = blob[off:off + n]
            diff = sum(x != y for x, y in zip(got, want)) + abs(n - size)
            near = max(0.0, 1.0 - diff / size) if size else 0.0
            rows.append((near, name, e["address"], n, size, diff, src))

    ok = [r for r in rows if r[0] is not None]
    bad = [r for r in rows if r[0] is None]
    ok.sort(key=lambda r: -r[0])

    print(f"{'function':<20}{'address':<12}{'size':>12}{'diff':>7}{'closeness':>11}  source")
    print("-" * 104)
    for near, name, addr, n, size, diff, src in ok:
        print(f"{name:<20}{addr:<12}{n:>5}/{size:<6}{diff:>7}{near*100:>9.1f}%  "
              f"{src.replace('src/','')}")
    for _, name, addr, _, _, why, src in bad:
        print(f"{name:<20}{addr:<12}{'-':>12}{'-':>7}{'-':>10}  {src} ({why})")

    if ok:
        print(f"\n{len(ok)} non-matching functions, {sum(r[4] for r in ok)} bytes in total")
        print(f"with difference <= 16: {sum(1 for r in ok if r[5] <= 16)}")

    if a.csv:
        out = Path(a.csv)
        out.parent.mkdir(parents=True, exist_ok=True)
        with out.open("w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["name", "address", "compiled_size", "mapped_size",
                        "diff", "nearness", "source"])
            for near, name, addr, n, size, diff, src in ok:
                w.writerow([name, addr, n, size, diff, f"{near:.4f}", src])
        print(f"yazildi: {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
