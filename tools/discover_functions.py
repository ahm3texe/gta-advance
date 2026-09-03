#!/usr/bin/env python3
"""Ghidra'nin kacirdigi fonksiyonlari fonksiyon haritasindaki bosluklarda arar.

data/functions.csv Ghidra'nin otomatik analizinden geliyor ve bu ROM'da
eksik oldugu kanitlandi: havuzdaki isleyici isaretcilerini cozerken elle
bes fonksiyon bulundu (0x0803DEA8, 0x0803E280, 0x0803F0E8, 0x080397EC...).
Bu arac ayni isi sistematik yapar.

YONTEM: bilinen fonksiyonlarin arasindaki bosluklarda Thumb prologu
(`push {..., lr}`) arar, sonra audit_boundaries yurutucusuyle govdeyi
cikarir. Dort siki kosul:
  1. govde 8-4096 bayt ve cozulemeyen dolayli atlama yok
  2. govde bosluktan tasmiyor (bilinen fonksiyona girmiyor)
  3. onunde bir bitirici (`bx lr` / `pop {..,pc}` / `bx rN`) ya da
     hizalama dolgusu var -- yani gercekten bir fonksiyon sinirinda
  4. birbirini kapsayan adaylardan yalnizca disi alinir

Bulunanlar `discovered` durumuyla eklenir: adres ve sinir otomatiktir,
govdeleri henuz incelenmedi.

Kullanim:
    python3 tools/discover_functions.py            # yalnizca rapor
    python3 tools/discover_functions.py --apply    # functions.csv'ye ekle
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_boundaries import Walker, in_arm_range, ARM_FUNCTIONS  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000
MIN_SIZE, MAX_SIZE = 8, 4096


def main() -> None:
    apply = "--apply" in sys.argv
    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fields = list(rows[0].keys())

    known = sorted((int(r["address"], 16), int(r["size"] or 0)) for r in rows
                   if r["size"].strip())
    gaps, prev = [], None
    for address, size in known:
        if prev is not None and address > prev:
            gaps.append((prev, address))
        prev = max(prev or 0, address + size)

    def half(addr: int) -> int:
        off = addr - ROM_BASE
        return int.from_bytes(rom[off:off + 2], "little")

    found = []
    for gap_start, gap_end in gaps:
        addr = (gap_start + 1) & ~1
        while addr + 2 <= gap_end:
            word = half(addr)
            if (word & 0xFF00) == 0xB500 and half(addr + 2) not in (0x0000, 0xFFFF):
                if not in_arm_range(addr) and addr not in ARM_FUNCTIONS:
                    walker = Walker(rom, addr)
                    walker.run()
                    size = walker.code_extent()
                    prev_word = half(addr - 2) if addr - 2 >= gap_start else 0x4770
                    terminated = (prev_word == 0x4770
                                  or (prev_word & 0xFF00) == 0xBD00
                                  or (prev_word & 0xFF87) == 0x4700
                                  or prev_word == 0x0000)
                    if (MIN_SIZE <= size <= MAX_SIZE and not walker.unresolved
                            and addr + size <= gap_end and terminated):
                        found.append((addr, size))
            addr += 2

    # Kapsayanlari ele: bir fonksiyonun govdesi icindeki prolog deseni
    # ayri bir fonksiyon degildir.
    found.sort()
    unique, last_end = [], 0
    for addr, size in found:
        if addr < last_end:
            continue
        unique.append((addr, size))
        last_end = addr + size

    total = sum(size for _, size in unique)
    print(f"Bosluk: {len(gaps)} adet")
    print(f"Bulunan fonksiyon: {len(unique)}, toplam {total} bayt\n")
    print(f"{'adres':12} {'bayt':>6}")
    print("-" * 20)
    for addr, size in sorted(unique, key=lambda f: -f[1])[:15]:
        print(f"0x{addr:08X} {size:>6}")

    if not apply:
        print("\n(yalnizca rapor; eklemek icin --apply)")
        return

    for addr, size in unique:
        rows.append({
            "address": f"0x{addr:08X}", "name": f"FUN_{addr:08x}",
            "size": str(size), "status": "discovered", "module": "unknown",
            "notes": "Ghidra kacirmisti; tools/discover_functions.py ile "
                     "bulundu, govdesi henuz incelenmedi",
        })
    rows.sort(key=lambda r: int(r["address"], 16))
    with FUNCTIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nfunctions.csv: {len(unique)} yeni fonksiyon eklendi.")


if __name__ == "__main__":
    main()
