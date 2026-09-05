#!/usr/bin/env python3
"""Eslesmeyen her C kaynagini ROM ile karsilastirip YAKINLIK raporu verir.

Neden var
---------
`make progress` yalnizca eslesti/eslesmedi soyluyor; hangi fonksiyonun bir
bayt, hangisinin yuzlerce bayt uzakta oldugu hicbir yerde tutulmuyordu.
Sirada ne oldugunu secmek icin gereken sey tam olarak bu.

Olculen degerler
----------------
  boyut     derlenmis boyut / haritadaki boyut
  fark      ayni ofsette farkli bayt sayisi + boyut farkinin mutlak degeri
  yakinlik  1 - fark/haritadaki boyut  (boyut tutmuyorsa da anlamli kalir)

DIKKAT: yakinlik yuzdesi KABA bir siralama olcutudur, ilerleme olcusu
degil.  Tek bir yazmac farki tum fonksiyonu kaydirabilir ve yuzde aniden
duser; tersine yuksek yuzde son baytin kolay kapanacagi anlamina gelmez.
Gercek sinyal fark SAYISININ kucuklugudur.

Kullanim:
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
    ap.add_argument("--csv", help="sonucu bu dosyaya da yaz")
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
                rows.append((None, name, e["address"], 0, 0, "linkte yok", src))
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

    print(f"{'fonksiyon':<20}{'adres':<12}{'boyut':>12}{'fark':>7}{'yakinlik':>10}  kaynak")
    print("-" * 104)
    for near, name, addr, n, size, diff, src in ok:
        print(f"{name:<20}{addr:<12}{n:>5}/{size:<6}{diff:>7}{near*100:>9.1f}%  "
              f"{src.replace('src/','')}")
    for _, name, addr, _, _, why, src in bad:
        print(f"{name:<20}{addr:<12}{'-':>12}{'-':>7}{'-':>10}  {src} ({why})")

    if ok:
        print(f"\n{len(ok)} eslesmeyen fonksiyon, toplam {sum(r[4] for r in ok)} bayt")
        print(f"fark <= 16 olan: {sum(1 for r in ok if r[5] <= 16)}")

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
