#!/usr/bin/env python3
"""Bilinmeyen fonksiyonlara ROM bitisikligiyle modul atar.

Gerekce: baglayici ayni ceviri biriminin (.c dosyasi) fonksiyonlarini ROM'da
ARDISIK yerlestirir.  Dolayisiyla bilinen iki komsunun ARASINDA kalan
bilinmeyenler buyuk olasilikla ayni module aittir.

MUHAFAZAKAR KURAL: bir bosluga yalnizca ONCESI ve SONRASI AYNI modulse
atama yapilir.  Taraflar farkliysa ya da bir taraf yoksa DOKUNULMAZ --
uydurma yapmaktansa 'unknown' birakmak yeglenir.

Atanan degerler CIKARIM'dir, olcum degil; notes alanina isaretlenir.

Kullanim:
    python3 tools/infer_modules.py --dry-run
    python3 tools/infer_modules.py
"""
import argparse
import csv
import pathlib
from collections import Counter

ROOT = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = ROOT / "data" / "functions.csv"
MARK = "MODUL CIKARIMI (tools/infer_modules.py): iki yanindaki bilinen komsu da"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--max-gap", type=int, default=24,
                    help="bir boslukta en fazla kac bilinmeyen doldurulur")
    args = ap.parse_args()

    rows = list(csv.DictReader(FUNCTIONS.open()))
    fields = list(rows[0].keys())
    ordered = sorted(
        (r for r in rows if r["address"].startswith("0x")),
        key=lambda r: int(r["address"], 16),
    )

    filled, skipped_diff, skipped_long = 0, 0, 0
    i = 0
    while i < len(ordered):
        if ordered[i]["module"] != "unknown":
            i += 1
            continue
        start = i
        while i < len(ordered) and ordered[i]["module"] == "unknown":
            i += 1
        before = ordered[start - 1]["module"] if start > 0 else None
        after = ordered[i]["module"] if i < len(ordered) else None
        gap = i - start

        if before is None or after is None or before != after:
            skipped_diff += gap
            continue
        if gap > args.max_gap:
            skipped_long += gap
            continue

        for r in ordered[start:i]:
            r["module"] = before
            note = f"{MARK} '{before}'."
            if MARK not in r["notes"]:
                r["notes"] = (r["notes"] + "  " if r["notes"] else "") + note
        filled += gap

    total_unknown = filled + skipped_diff + skipped_long
    print(f"bilinmeyen fonksiyon      : {total_unknown}")
    print(f"  DOLDURULDU (iki yan ayni): {filled}")
    print(f"  atlandi (yanlar farkli)  : {skipped_diff}")
    print(f"  atlandi (boslugu {args.max_gap}'ten uzun): {skipped_long}")

    if filled:
        after_counts = Counter(r["module"] for r in rows)
        print("\nyeni dagilim:")
        for k, v in sorted(after_counts.items(), key=lambda x: -x[1]):
            print(f"  {k:<16} {v}")

    if args.dry_run:
        print("\nhicbir sey yazilmadi (--dry-run)")
        return 0

    with FUNCTIONS.open("w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    print(f"\n{FUNCTIONS.relative_to(ROOT)} yazildi")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
