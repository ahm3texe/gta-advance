#!/usr/bin/env python3
"""Kural 45'in gecerli oldugu DAGITIM ZINCIRLERINI ROM'dan bulur.

Neden ROM'dan
-------------
Ilk denemede bu tarama Ghidra ciktisi uzerinden, "birbirinin ayni ard arda
ifade bloklari" sayilarak yapildi.  YANLIS POZITIF veriyor: FUN_080108f4'te
139 tekrar sayildi ama o fonksiyon tekrar eden DAL GOVDESI degil, tekrar
eden KURESEL ERISIM kalibi iceriyor (40 ayri DAT_ global, ic ice donguler).
Metin benzerligi yanlis olcut.

Dogru imza ROM tarafinda: kural 45 ancak AYNI YAZMACA karsi uzun bir
`cmp rX,#imm` zinciri varsa gecerlidir -- yani bir durum degiskenine gore
dallanan uzun if/else zinciri.  FUN_080260a8'de bu zincir 12 halkaliydi ve
dal govdeleri tekrar ediyordu; ortak yerel verilince agbcc bir dalin
blogunu tumuyle yuttu (bkz. docs/COMPILER.md kural 45).

Bu arac o zinciri dogrudan makine kodundan sayar.

KAPSAM SINIRI
-------------
Havuz kelimeleri de tesadufen `cmp` gibi kodlanabilir; bu yuzden zincirin
ayni yazmaca karsi olmasi ve halkalar arasinda makul bosluk bulunmasi
sarti aranir.  Yine de aday listesi ELENMIS degil, INCELENMEYE degerdir.

Kullanim:
    python3 tools/scan_dispatch.py            # eslesmemis, >=512 bayt
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
    """Ayni yazmaca karsi ard arda gelen `cmp rX,#imm` halkalarini dondurur."""
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
    ap.add_argument("--min", type=int, default=5, help="en az kac halka")
    ap.add_argument("--size", type=int, default=512, help="en kucuk fonksiyon boyutu")
    ap.add_argument("--distinct", type=int, default=5,
                    help="zincirde en az kac FARKLI sifirdan buyuk deger")
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
        # AYIRT EDICI OLCUT: zincirdeki FARKLI SIFIRDAN BUYUK degerler.
        # `cmp rX,#0` halkalari bos kontroldur, dagitim degil; yalnizca
        # sifir olmayan ve birbirinden farkli degerler bir durum
        # degiskenine gore dallanmayi gosterir.
        distinct = len({v for v in vals if v})
        if len(ch) >= a.min and distinct >= a.distinct:
            rows.append((distinct, len(ch), size, r["name"], r["address"], vals))
    rows.sort(reverse=True)

    print(f"{'fonksiyon':<20}{'adres':<12}{'boyut':>6}{'farkli':>7}{'halka':>7}  degerler")
    print("-" * 100)
    for distinct, n, size, name, addr, vals in rows:
        v = ", ".join(str(x) for x in vals[:12])
        if len(vals) > 12:
            v += ", ..."
        print(f"{name:<20}{addr:<12}{size:>6}{distinct:>7}{n:>7}  {v}")
    print(f"\n{len(rows)} aday, toplam {sum(r[2] for r in rows)} bayt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
