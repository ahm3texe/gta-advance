#!/usr/bin/env python3
"""ROM'daki saf iletme sarmalayicilarini bulur.

Kalip (10 bayt): push {lr} / bl HEDEF / pop {rX} / bx rX
Govde tek cagridan ibaret oldugu icin C karsiligi mekanik:
    void  f(void) { HEDEF(); }        <- pop {r0}; bx r0
    u32   f(void) { return HEDEF(); } <- pop {r1}; bx r1  (r0 donus tasiyor)

Kullanim: python3 tools/find_wrappers.py [--all]
  varsayilan: yalnizca HENUZ ESLESMEYEN kayitlar
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_BASE = 0x08000000


def main() -> None:
    rom = (ROOT / "baserom.gba").read_bytes()
    rows = list(csv.DictReader((ROOT / "data/functions.csv").open(newline="",
                                                                 encoding="utf-8")))
    show_all = "--all" in sys.argv

    def hw(a: int) -> int:
        o = a - ROM_BASE
        return int.from_bytes(rom[o:o + 2], "little")

    found = []
    for row in rows:
        if not show_all and row["status"] == "matching":
            continue
        addr = int(row["address"], 16)
        size = int(row["size"] or 0)
        if size < 10:
            continue
        if hw(addr) != 0xB500:                      # push {lr}
            continue
        w1, w2 = hw(addr + 2), hw(addr + 4)
        if not (0xF000 <= w1 <= 0xF7FF and 0xF800 <= w2 <= 0xFFFF):
            continue
        pop, bx = hw(addr + 6), hw(addr + 8)
        if pop == 0xBC01 and bx == 0x4700:
            returns = False
        elif pop == 0xBC02 and bx == 0x4708:
            returns = True
        else:
            continue
        off = ((w1 & 0x7FF) << 12) | ((w2 & 0x7FF) << 1)
        if off & 0x400000:
            off -= 0x800000
        # `bl` komutu addr+2'de (push {lr} 2 bayt), Thumb'da PC =
        # komut adresi + 4, yani taban addr+6. addr+4 yazmak hedefi tam
        # iki bayt geriye kaydiriyordu (objdump ile karsilastirip yakalandi).
        found.append((addr, addr + 6 + off, returns, row["name"], size))

    print(f"{len(found)} iletme sarmalayicisi")
    for addr, target, returns, name, size in found:
        kind = "u32" if returns else "void"
        print(f"0x{addr:08X} {size:>3}B {kind:4} -> 0x{target:08X}  {name}")


if __name__ == "__main__":
    main()
