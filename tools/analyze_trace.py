#!/usr/bin/env python3
"""Summarize build/trace.log: changes per symbol, value transitions, pointer resolution.

Usage:
    python3 tools/analyze_trace.py                 # summary of the last session
    python3 tools/analyze_trace.py --list          # all sessions in the log
    python3 tools/analyze_trace.py --session 2     # examine session 2
    python3 tools/analyze_trace.py gSessionPtr     # transitions of one symbol
"""
import csv
import pathlib
import re
import sys
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parent.parent
LOG = ROOT / "build" / "trace.log"
LINE = re.compile(r"^f(\d+)\s+(\S+)\s+(.*)$")

REGIONS = {0x02: "EWRAM", 0x03: "IWRAM", 0x05: "PALET",
           0x06: "VRAM", 0x07: "OAM", 0x08: "ROM"}


def ram_symbols():
    path = ROOT / "data" / "ram_map.csv"
    return {int(r["address"], 16): r["name"] for r in csv.DictReader(path.open())}


def classify(value, syms):
    """Try to interpret a value as an address."""
    if value in syms:
        return f"= {syms[value]} (KNOWN SYMBOL)"
    region = REGIONS.get(value >> 24)
    return f"-> {region}" if region else ""


def sessions(lines):
    """The log holds several sessions; each is returned separately."""
    marks = [i for i, l in enumerate(lines) if "new session" in l]
    if not marks:
        return [lines]
    bounds = marks + [len(lines)]
    return [lines[bounds[i]:bounds[i + 1]] for i in range(len(marks))]


def pick_session(lines, index):
    """index: None -> the last one, a 1-based number -> that session."""
    blocks = sessions(lines)
    if index is None:
        return blocks[-1], len(blocks), len(blocks)
    if not 1 <= index <= len(blocks):
        raise SystemExit(f"no session {index}; the log holds {len(blocks)} sessions")
    return blocks[index - 1], index, len(blocks)


def main(argv):
    if not LOG.exists():
        print(f"no log: {LOG}")
        return 1
    lines = LOG.read_text(errors="replace").splitlines()
    idx = None
    if "--session" in argv:
        idx = int(argv[argv.index("--session") + 1])
        argv = [a for i, a in enumerate(argv)
                if i not in (argv.index("--session"), argv.index("--session") + 1)]
    sess, used, total = pick_session(lines, idx)
    print(f"[session {used}/{total}]")
    events = [(int(m.group(1)), m.group(2), m.group(3))
              for m in (LINE.match(l) for l in sess) if m]
    if not events:
        print("no change records in this session.")
        for l in sess:
            if "ERROR" in l or "warning" in l or "STARTED" in l or "error" in l:
                print(" ", l)
        return 1

    syms = ram_symbols()

    if len(argv) > 1 and argv[1] == "--list":
        blocks = sessions(lines)
        print(f"{len(blocks)} sessions in the log:")
        for i, b in enumerate(blocks, 1):
            ev = [l for l in b if LINE.match(l)]
            frames = [int(LINE.match(l).group(1)) for l in ev] or [0]
            print(f"  session {i}: {len(ev):>5} events, frames {min(frames)}-{max(frames)}")
        return 0

    if len(argv) > 1:
        name = argv[1]
        rows = [(f, r) for f, n, r in events if n == name]
        if not rows:
            print(f"{name}: no changes in this session")
            return 0
        print(f"=== {name} ({len(rows)} changes) ===")
        for frame, raw in rows:
            note = ""
            m = re.match(r"^(\S+) -> (\S+)$", raw)
            if m and m.group(2).isdigit():
                note = classify(int(m.group(2)), syms)
            print(f"  f{frame:<7} {raw} {note}")
        return 0

    suppressed = [(f, n) for f, n, r in events if "SUPPRESSED" in r]
    counts, first = defaultdict(int), {}
    for f, n, r in events:
        if "SUPPRESSED" in r:
            continue
        counts[n] += 1
        first.setdefault(n, f)

    frames = [f for f, _, _ in events]
    print(f"{len(events)} events, frames {frames[0]}-{frames[-1]}, "
          f"{len(counts)} symbols, {len(suppressed)} suppressed\n")
    print("--- changes per symbol (most -> least) ---")
    for n, c in sorted(counts.items(), key=lambda x: (-x[1], first[x[0]])):
        print(f"  {c:>4}x  first f{first[n]:<7} {n}")
    if suppressed:
        print("\n--- suppressed for exceeding the noise threshold ---")
        for f, n in suppressed:
            print(f"  f{f:<7} {n}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
