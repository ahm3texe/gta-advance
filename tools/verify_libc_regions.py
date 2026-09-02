#!/usr/bin/env python3
"""ROM'daki standart kutuphane bolgelerini agbcc'nin libc.a'sina karsi dogrular.

ROM, agbcc ile birlikte gelen newlib'e linkleniyor (docs/ROM_ARTIFACTS.md).
Bu bolgeler tersine muhendislik gerektirmez: kaynagi zaten elimizde ve
dogrulama, kutuphane govdesinin ROM'daki byte'larla birebir ayni oldugunu
gostermektir.

Yalnizca YER DEGISTIRMESIZ fonksiyonlar dogrulanabilir; dis cagri iceren
govdeler linklenmeden ROM byte'lariyla eslesmez.

Kullanim:  python3 tools/verify_libc_regions.py
"""
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIBC = ROOT / "tools/agbcc/lib/libc.a"
ROM = ROOT / "baserom.gba"
REGIONS = ROOT / "data/libc_regions.csv"
ROM_BASE = 0x08000000

GREEN, RED, RESET = "\033[32m", "\033[31m", "\033[0m"


def main() -> None:
    if not LIBC.exists():
        sys.exit("tools/agbcc/lib/libc.a yok. Once: make agbcc")
    if not REGIONS.exists():
        sys.exit(f"{REGIONS} yok.")

    rom = ROM.read_bytes()
    work = Path(tempfile.mkdtemp())
    subprocess.run(["arm-none-eabi-ar", "x", str(LIBC)], cwd=work, capture_output=True)
    binary = work / "slice.bin"

    total, ok = 0, 0
    for row in csv.DictReader(REGIONS.open(newline="", encoding="utf-8")):
        total += 1
        obj = work / row["object"]
        name = row["symbol"]
        address = int(row["address"], 16)

        nm = subprocess.run(["arm-none-eabi-nm", "-S", str(obj)],
                            capture_output=True, text=True).stdout
        entry = next(
            (p for p in (line.split() for line in nm.splitlines())
             if len(p) == 4 and p[2] in "tT" and p[3] == name),
            None,
        )
        if entry is None:
            print(f"{RED}EKSIK{RESET}: {name} {row['object']} icinde yok")
            continue
        offset, size = int(entry[0], 16), int(entry[1], 16)

        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(binary)],
                       capture_output=True)
        body = binary.read_bytes()[offset:offset + size]
        start = address - ROM_BASE
        if body == rom[start:start + size]:
            ok += 1
            print(f"MATCH: libc.a({row['object']}):{name} == "
                  f"baserom.gba[0x{start:X}:0x{start + size:X}] ({size} bytes)")
        else:
            print(f"{RED}MISMATCH{RESET}: {name} @ 0x{address:08X} ({size} bytes)")

    verified = sum(
        int(r["end"], 16) - int(r["address"], 16)
        for r in csv.DictReader(REGIONS.open(newline="", encoding="utf-8"))
    )
    print(f"LIBC REGION: {ok}/{total} parca, {verified} ROM byte")
    sys.exit(0 if ok == total else 1)


if __name__ == "__main__":
    main()
