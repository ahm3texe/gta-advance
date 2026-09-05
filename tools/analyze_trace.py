#!/usr/bin/env python3
"""build/trace.log'u ozetler: sembol basina degisim, deger gecisleri, isaretci cozumu.

Kullanim:
    python3 tools/analyze_trace.py              # son oturumun ozeti
    python3 tools/analyze_trace.py gSessionPtr  # tek sembolun gecisleri
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
    """Bir degeri adres olarak yorumlamayi dene."""
    if value in syms:
        return f"= {syms[value]} (BILINEN SEMBOL)"
    region = REGIONS.get(value >> 24)
    return f"-> {region}" if region else ""


def last_session(lines):
    marks = [i for i, l in enumerate(lines) if "yeni oturum" in l]
    return lines[marks[-1]:] if marks else lines


def main(argv):
    if not LOG.exists():
        print(f"log yok: {LOG}")
        return 1
    lines = LOG.read_text(errors="replace").splitlines()
    sess = last_session(lines)
    events = [(int(m.group(1)), m.group(2), m.group(3))
              for m in (LINE.match(l) for l in sess) if m]
    if not events:
        print("bu oturumda degisim kaydi yok.")
        for l in sess:
            if "HATA" in l or "uyari" in l or "CALISTI" in l or "hata" in l:
                print(" ", l)
        return 1

    syms = ram_symbols()

    if len(argv) > 1:
        name = argv[1]
        rows = [(f, r) for f, n, r in events if n == name]
        if not rows:
            print(f"{name}: bu oturumda degisim yok")
            return 0
        print(f"=== {name} ({len(rows)} degisim) ===")
        for frame, raw in rows:
            note = ""
            m = re.match(r"^(\S+) -> (\S+)$", raw)
            if m and m.group(2).isdigit():
                note = classify(int(m.group(2)), syms)
            print(f"  f{frame:<7} {raw} {note}")
        return 0

    suppressed = [(f, n) for f, n, r in events if "SUSTURULDU" in r]
    counts, first = defaultdict(int), {}
    for f, n, r in events:
        if "SUSTURULDU" in r:
            continue
        counts[n] += 1
        first.setdefault(n, f)

    frames = [f for f, _, _ in events]
    print(f"olay {len(events)}, kare {frames[0]}-{frames[-1]}, "
          f"sembol {len(counts)}, susturulan {len(suppressed)}\n")
    print("--- sembol basina degisim (cok -> az) ---")
    for n, c in sorted(counts.items(), key=lambda x: (-x[1], first[x[0]])):
        print(f"  {c:>4}x  ilk f{first[n]:<7} {n}")
    if suppressed:
        print("\n--- gurultu esigini asip susturulanlar ---")
        for f, n in suppressed:
            print(f"  f{f:<7} {n}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
