#!/usr/bin/env python3
"""Komsulugu eslesen, henuz C'si YAZILMAMIS fonksiyonlari secer.

NEDEN
-----
Bir fonksiyonun etrafindaki fonksiyonlar zaten eslesmisse, o bolgenin tip
sozlugu (struct yerlesimleri, RAM sembolleri, cagri imzalari) hazir demektir;
bu adaylar tipik olarak 1-2 denemede kapaniyor.  Bu secim daha once elle
(tek seferlik python parcaciklariyla) yapiliyordu ve DEPODA KAYITLI DEGILDI,
yani plandaki sayilar yeniden uretilemiyordu.  Bu arac o bosluğu kapatiyor.

Bu bir TESHIS aracidir: sirf komsulugu yogun diye bir fonksiyon kolay
degildir.  Cikan liste `tools/diff_function.py` ile dogrulanmadan hicbir sey
kaydedilmez.

OLCUT (hepsi degistirilebilir)
------------------------------
  - status != matching
  - ARM DEGIL (notlarda ayri sozcuk olarak "ARM" gecmiyor) -- agbcc_arm
    8 yazmac tavani yuzunden ARM kapali (docs/STATUS.md)
  - src/ altinda AYNI ADLA bir tanim YOK, yani taslagi bile yazilmamis
    (--include-drafts ile bu eleme kapatilir; yakin iskalar da listelenir)
  - boyut [--min-size, --max-size] araliginda
  - adres sirasinda 9'luk pencerede (i-4 .. i+4) en az --neighbours tane
    eslesen fonksiyon var

Kullanim:
    python3 tools/find_neighbour_dense.py                 # varsayilan secim
    python3 tools/find_neighbour_dense.py --neighbours 4 --min-size 48 \
        --max-size 512 --include-drafts                   # ilk analizdeki secim
    python3 tools/find_neighbour_dense.py --out data/neighbour_dense.csv
"""
import argparse
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS = ROOT / "data/functions.csv"

# `int foo(...) {` / `static void *bar(...)\n{` gibi TANIMLARI yakalar;
# bildirimleri (`;` ile biten) disarida birakir.
DEFINITION = re.compile(r"^\w[\w\s\*]*?\b(\w+)\s*\([^;]*\)\s*\{", re.M)


def defined_in_sources() -> set[str]:
    """src/ altinda govdesi yazilmis fonksiyon adlari."""
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
                    help="9'luk pencerede gereken en az eslesen sayisi (varsayilan 3)")
    ap.add_argument("--min-size", type=int, default=40)
    ap.add_argument("--max-size", type=int, default=520)
    ap.add_argument("--include-drafts", action="store_true",
                    help="C taslagi olanlari da listele (yakin iskalar)")
    ap.add_argument("--include-arm", action="store_true")
    ap.add_argument("--limit", type=int, default=30)
    ap.add_argument("--out", help="secimi CSV olarak yaz (depoda kayit birakir)")
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

    print(f"olcut: boyut {args.min_size}-{args.max_size}, komsu >= {args.neighbours}, "
          f"taslagi olanlar {'DAHIL' if args.include_drafts else 'HARIC'}, "
          f"ARM {'DAHIL' if args.include_arm else 'HARIC'}")
    print(f"{len(picks)} aday / {bytes_total} bayt "
          f"({100 * bytes_total / 454258:.2f}% ROM kodu)\n")
    print(f"{'adres':<12} {'bayt':>5} {'komsu':>6}  ad")
    print("-" * 62)
    for near, size, row in picks[:args.limit]:
        print(f"{row['address']:<12} {size:>5} {near:>6}  {row['name']}")
    if len(picks) > args.limit:
        print(f"... {len(picks) - args.limit} aday daha (--limit ile arttir)")

    if args.out:
        out = ROOT / args.out
        with out.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle)
            writer.writerow(["address", "name", "size", "neighbours", "status"])
            for near, size, row in picks:
                writer.writerow([row["address"], row["name"], size, near, row["status"]])
        print(f"\n-> {args.out} ({len(picks)} satir)")


if __name__ == "__main__":
    main()
