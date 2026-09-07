#!/usr/bin/env python3
"""Yapisal olarak AYNI olan fonksiyon ciftlerini bulur.

NEDEN
-----
0x08031844 (472 bayt) bir ajanin 341 bin jetonunu yedi ve "yazmac dagitimi,
kaynak duzeyinde kaldirac yok" diye kapatildi. Oysa eslesen kardesi
0x08031A1C ROM'da duruyordu ve iki govdenin TEK farki bir karsilastirmanin
kutbuydu (`blt` <-> `bge`). Eslesen kardesin kaynagini kopyalayip o testi
cevirmek ilk denemede tam eslesme verdi (docs/WORKFLOW.md §10).

Bu arac o aramayi elle yapmayi birakip tarama haline getiriyor.

NASIL
-----
Her fonksiyonun govdesi 16 bitlik yarim sozlere bolunup NORMALLESTIRILIYOR:
konuma bagli alanlar (dal uzakligi, havuz uzakligi) sifirlaniyor, komutun
kimligi ve YAZMACLARI korunuyor. Iki fonksiyon ayni C'den gelmisse
normallestirilmis diziler birbirine cok yakin olur; ROM'daki yerleri farkli
oldugu icin ham baytlar tutmaz.

Bu bir TESHIS aracidir, kanit degil. Cikan cift `tools/diff_function.py` ile
dogrulanmadan hicbir sey kaydedilmez.

Kullanim:
    python3 tools/find_twins.py                # eslesmeyen -> eslesen
    python3 tools/find_twins.py --unmatched    # eslesmeyen -> eslesmeyen (kumeler)
    python3 tools/find_twins.py --min 0.90     # esik (varsayilan 0.85)
"""
import argparse
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000


def normalize(body: bytes) -> tuple[int, ...]:
    """Yarim sozleri konumdan bagimsiz hale getir.

    Sifirlanan alanlar yalnizca ADRESE bagli olanlardir; yazmac numaralari
    ve komut kimligi dokunulmadan kalir, cunku ayirt edici olan onlar.
    """
    out = []
    for i in range(0, len(body) - 1, 2):
        hw = body[i] | (body[i + 1] << 8)
        top5 = hw >> 11
        if top5 == 0b11100:            # B <imm11>
            hw &= 0xF800
        elif (hw >> 12) == 0b1101:     # B<cond> <imm8> — kosul korunur
            hw &= 0xFF00
        elif top5 in (0b11110, 0b11111):  # BL cifti
            hw &= 0xF800
        elif (hw >> 11) == 0b01001:    # LDR Rd,[PC,#imm8] — havuz uzakligi
            hw &= 0xF800 | 0x0700
        elif (hw >> 11) == 0b10101:    # ADD Rd,PC,#imm8
            hw &= 0xF800 | 0x0700
        out.append(hw)
    return tuple(out)


def similarity(a: tuple[int, ...], b: tuple[int, ...]) -> float:
    """Ayni uzunluktaki iki dizide birebir ayni konum orani."""
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
        if size < 24:                  # kucuk saplamalarda benzerlik anlamsiz
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
                    help="eslesmeyenleri birbiriyle karsilastir (kume bulur)")
    ap.add_argument("--limit", type=int, default=40)
    args = ap.parse_args()

    funcs = load()
    left = [f for f in funcs if f["status"] != "matching"]
    right = left if args.unmatched else [f for f in funcs if f["status"] == "matching"]

    # Uzunluga gore kovala: yalnizca +-%10 uzunluktakiler karsilastirilir.
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
    kind = "eslesmeyen" if args.unmatched else "ESLESEN"
    print(f"{len(hits)} aday ({args.min:.0%} ve uzeri, hedef: {kind} kardes)\n")
    print(f"{'benzerlik':>9} {'bayt':>6}  {'eslesmeyen':<34} {'kardes':<34}")
    print("-" * 90)
    total = 0
    for s, a, b in hits[:args.limit]:
        total += a["size"]
        print(f"{s:>8.1%} {a['size']:>6}  {a['address']} {a['name'][:22]:<22} "
              f"{b['address']} {b['name'][:22]:<22}")
    print("-" * 90)
    print(f"listelenen {min(len(hits), args.limit)} adayin toplami: {total} bayt")
    print(f"tum adaylarin toplami: {sum(a['size'] for _, a, _ in hits)} bayt")


if __name__ == "__main__":
    main()
