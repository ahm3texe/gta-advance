#!/usr/bin/env python3
"""agbcc'nin libc.a'sindaki fonksiyonlari ROM icinde arar.

ROM, agbcc ile birlikte gelen newlib'e linkleniyor: "bug in vfprintf: bad base",
"_sbrk: Heap and stack collision" ve "Infinity" dizeleri hem ROM'da hem
tools/agbcc/lib/libc.a icinde birebir var. Yani kod bolgesinin kuyrugu tersine
muhendislik gerektirmeyen standart kutuphane kodudur; kaynagi zaten elimizde.

Bu tarama yalnizca YER DEGISTIRMESIZ fonksiyonlari bulabilir. Dis cagri iceren
fonksiyonlar hedef adrese linklenmeden ROM'daki byte'lariyla eslesmez; onlar
icin adres tahmini gerekir (tools/agbcc_build.py linkleme katmani).

Kullanim:  python3 tools/scan_libc.py [--csv]
"""
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIBC = ROOT / "tools/agbcc/lib/libc.a"
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000
MIN_SIZE = 16  # kisa govdeler tesaduf eslesme uretir


def main() -> None:
    if not LIBC.exists():
        sys.exit("tools/agbcc/lib/libc.a yok. Once: make agbcc")
    if not ROM.exists():
        sys.exit("baserom.gba yok.")

    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        known = {int(r["address"], 16) for r in csv.DictReader(handle)}

    work = Path(tempfile.mkdtemp())
    subprocess.run(["arm-none-eabi-ar", "x", str(LIBC)], cwd=work, capture_output=True)
    binary = work / "slice.bin"

    found, checked, skipped = [], 0, 0
    for obj in sorted(work.glob("*.o")):
        nm = subprocess.run(["arm-none-eabi-nm", "-S", str(obj)],
                            capture_output=True, text=True).stdout
        symbols = [
            (p[3], int(p[0], 16), int(p[1], 16))
            for p in (line.split() for line in nm.splitlines())
            if len(p) == 4 and p[2] in "tT"
        ]
        if not symbols:
            continue
        relocs = subprocess.run(["arm-none-eabi-objdump", "-r", str(obj)],
                                capture_output=True, text=True).stdout
        if "RELOCATION RECORDS FOR [.text]" in relocs:
            skipped += len(symbols)
            continue
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(binary)],
                       capture_output=True)
        text = binary.read_bytes()
        for name, offset, size in symbols:
            if size < MIN_SIZE or offset + size > len(text):
                continue
            checked += 1
            index = rom.find(text[offset:offset + size])
            if index >= 0:
                found.append((ROM_BASE + index, name, size))

    found.sort()
    if "--csv" in sys.argv:
        print("address,name,status,module,notes")
        for address, name, size in found:
            note = "newlib routine; body byte-identical to agbcc libc.a build"
            if address not in known:
                note += "; missed by Ghidra auto-analysis"
            print(f'0x{address:08X},{name},documented,libc,"{note}"')
        return

    print(f"libc.a: {checked} fonksiyon arandi "
          f"({skipped} tanesi yer degistirmeli, linkleme gerektiriyor)")
    print(f"ROM'da birebir bulunan: {len(found)}\n")
    for address, name, size in found:
        flag = "" if address in known else "   <- Ghidra kacirmis"
        print(f"  0x{address:08X}  {name:20} {size:>4}B{flag}")


if __name__ == "__main__":
    main()
