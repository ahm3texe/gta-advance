#!/usr/bin/env python3
"""ROM'daki basit yaprak erisimcileri bulur (okuma VE yazma).

Kalip (6 baytlik cekirdek, ardindan hizalama + havuz):
    ldr rN, [pc, #imm]      <- havuzdan taban adres
    ldr|ldrb|ldrh rD, [rN, #ofs]      (getirici)
      ya da
    str|strb|strh rD, [rN, #ofs]      (koyucu; deger r0'da gelir, taban r1'e
                                       yuklenir cunku r0 dolu)
    bx lr

C karsiligi mekanik:
    u32  f(void)      { return sembol.alan; }
    void f(u32 value) { sembol.alan = value; }

Kullanim: python3 tools/find_accessors.py [--all]
  varsayilan: yalnizca HENUZ ESLESMEYEN kayitlar
  --all: dogrulama icin eslesenleri de gosterir
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_BASE = 0x08000000

# (maske, deger, tur, olcek) -- ofset olceklemesi Thumb kodlamasindan gelir
ACCESS = [
    # (maske, deger, tur, olcek, yon)
    (0xF800, 0x6800, "u32", 4, "get"),
    (0xF800, 0x7800, "u8", 1, "get"),
    (0xF800, 0x8800, "u16", 2, "get"),
    (0xF800, 0x6000, "u32", 4, "set"),
    (0xF800, 0x7000, "u8", 1, "set"),
    (0xF800, 0x8000, "u16", 2, "set"),
]


def main() -> None:
    rom = (ROOT / "baserom.gba").read_bytes()
    rows = list(csv.DictReader((ROOT / "data/functions.csv").open(newline="",
                                                                 encoding="utf-8")))
    ram = {int(r["address"], 16): r["name"]
           for r in csv.DictReader((ROOT / "data/ram_map.csv").open(newline="",
                                                                   encoding="utf-8"))}
    show_all = "--all" in sys.argv

    def hw(a: int) -> int:
        o = a - ROM_BASE
        return int.from_bytes(rom[o:o + 2], "little")

    def word(a: int) -> int:
        o = a - ROM_BASE
        return int.from_bytes(rom[o:o + 4], "little")

    found = []
    for row in rows:
        if not show_all and row["status"] == "matching":
            continue
        addr = int(row["address"], 16)
        if int(row["size"] or 0) < 8:
            continue
        h0, h1, h2 = hw(addr), hw(addr + 2), hw(addr + 4)
        if (h0 & 0xF800) != 0x4800:                 # ldr rN, [pc, #imm]
            continue
        if h2 != 0x4770:                            # bx lr
            continue
        kind = scale = direction = None
        for mask, value, k, sc, d in ACCESS:
            if (h1 & mask) == value:
                kind, scale, direction = k, sc, d
                break
        if kind is None:
            continue
        base_reg = (h0 >> 8) & 7
        if (h1 & 7) != base_reg and ((h1 >> 3) & 7) != base_reg:
            continue
        pool = ((addr + 4) & ~3) + ((h0 & 0xFF) * 4)
        sym = word(pool)
        offset = ((h1 >> 6) & 0x1F) * scale
        found.append((addr, sym, offset, kind, direction, ram.get(sym), row["name"]))

    gets = sum(1 for f in found if f[4] == "get")
    print(f"{len(found)} yaprak erisimci ({gets} getirici, {len(found) - gets} koyucu)")
    for addr, sym, off, kind, direction, name, fname in found:
        label = name or "(ram_map'te yok)"
        print(f"0x{addr:08X}  {direction:3} {kind:3} 0x{sym:08X}+0x{off:02X}  "
              f"{label:22} {fname}")


if __name__ == "__main__":
    main()
