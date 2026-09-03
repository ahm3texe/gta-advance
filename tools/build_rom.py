#!/usr/bin/env python3
"""Tam ROM'u yeniden uretir ve SHA-1'ini orijinalle karsilastirir.

Dogrulanmis her bolge KENDI KAYNAGIMIZDAN uretilir; kalan alanlar
baserom.gba'dan oldugu gibi kopyalanir. Sonucun hash'i orijinalle ayni
cikmalidir.

Bu, bolge bolge karsilastirmadan DAHA GUCLU bir iddiadir: dilim dilim
kontroller bolge sinirlari yanlis tanimlanmis ya da iki bolge cakismis olsa
bile gecebilir; tam ROM karsilastirmasi gecemez.

Kullanim:  python3 tools/build_rom.py [--out out/gtaadvance.gba]
"""
import csv
import hashlib
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
REGIONS = ROOT / "data/matching_regions.csv"
LIBC_REGIONS = ROOT / "data/libc_regions.csv"
OUT = ROOT / "out/gtaadvance.gba"
ROM_BASE = 0x08000000

GREEN, RED, RESET = "\033[32m", "\033[31m", "\033[0m"


def libc_slice(work: Path, obj: str, symbol: str) -> bytes:
    nm = subprocess.run(["arm-none-eabi-nm", "-S", str(work / obj)],
                        capture_output=True, text=True).stdout
    entry = next(
        (p for p in (line.split() for line in nm.splitlines())
         if len(p) == 4 and p[2] in "tT" and p[3] == symbol),
        None,
    )
    if entry is None:
        sys.exit(f"HATA: {symbol} {obj} icinde yok")
    offset, size = int(entry[0], 16), int(entry[1], 16)
    binary = work / "slice.bin"
    subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", "--only-section=.text",
                    str(work / obj), str(binary)], capture_output=True)
    return binary.read_bytes()[offset:offset + size]


def main() -> None:
    out = OUT
    for arg in sys.argv[1:]:
        if arg.startswith("--out="):
            out = Path(arg.split("=", 1)[1])
    if not ROM.exists():
        sys.exit("baserom.gba yok. Once: make prepare-rom ROM_ZIP=...")

    original = ROM.read_bytes()
    image = bytearray(original)
    placed = 0
    covered = 0

    for row in csv.DictReader(REGIONS.open(newline="", encoding="utf-8")):
        binary = ROOT / row["binary"]
        if not binary.exists():
            sys.exit(f"HATA: {row['binary']} uretilmemis. Once: make matching")
        start = int(row["start"], 16) - ROM_BASE
        end = int(row["end"], 16) - ROM_BASE
        body = binary.read_bytes()
        if len(body) != end - start:
            sys.exit(f"HATA: {row['binary']} {len(body)} byte, bolge {end - start} byte")
        image[start:end] = body
        placed += 1
        covered += end - start

    if LIBC_REGIONS.exists():
        import tempfile
        work = Path(tempfile.mkdtemp())
        subprocess.run(["arm-none-eabi-ar", "x", str(ROOT / "tools/agbcc/lib/libc.a")],
                       cwd=work, capture_output=True)
        for row in csv.DictReader(LIBC_REGIONS.open(newline="", encoding="utf-8")):
            start = int(row["address"], 16) - ROM_BASE
            end = int(row["end"], 16) - ROM_BASE
            body = libc_slice(work, row["object"], row["symbol"])
            if len(body) != end - start:
                sys.exit(f"HATA: {row['symbol']} {len(body)} byte, bolge {end - start} byte")
            image[start:end] = body
            placed += 1
            covered += end - start

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(image)

    built = hashlib.sha1(image).hexdigest()
    expected = hashlib.sha1(original).hexdigest()
    print(f"{placed} bolge kendi kaynagimizdan yerlestirildi ({covered} byte).")
    print(f"Geri kalan {len(original) - covered} byte baserom.gba'dan kopyalandi.")
    print(f"beklenen SHA-1: {expected}")
    print(f"uretilen SHA-1: {built}")
    if built == expected:
        print(f"{GREEN}HASH BIREBIR AYNI{RESET} — dogrulanmis {covered} byte kendi\n  kaynagimizdan doğru yere oturuyor. Kalan {len(original) - covered} byte\n  baserom.gba'dan kopyalandi, yani bu TAM ROM'un kaynaktan uretildigi\n  anlamina GELMEZ; hibrit butunlestirme sinamasidir.  -> {out}")
        return
    diff = sum(1 for a, b in zip(image, original) if a != b)
    print(f"{RED}HASH TUTMUYOR: {diff} byte farkli.{RESET}")
    sys.exit(1)


if __name__ == "__main__":
    main()
