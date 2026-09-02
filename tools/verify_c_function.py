#!/usr/bin/env python3
"""Bir C dosyasini agbcc ile derler ve her fonksiyonu ROM ile karsilastirir.

Kullanim:  python3 tools/verify_c_function.py src/save/save_helpers.c [--cc=agbcc]

Fonksiyon adresleri data/functions.csv'den okunur. Bir fonksiyon C'den
byte-matching oldugunda, esdeger assembly kaynagi artik gereksizdir.
"""
import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AGBCC_DIR = ROOT / "tools/agbcc/bin"
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
BUILD = ROOT / "build/cmatch"

# ROM uzerinde dogrulanan derleyici ve bayraklar (docs/COMPILER.md).
# old_agbcc, agbcc'nin eski varyanti; save_helpers'da 5/6 fonksiyonu tutturan bu.
DEFAULT_CC = "old_agbcc"
CC1FLAGS = ["-mthumb-interwork", "-O2", "-fhex-asm"]
ROM_BASE = 0x08000000


def run(cmd: list[str], stdout: Path | None = None) -> None:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"HATA: {' '.join(cmd)}\n{result.stderr.strip()}")
    if stdout is not None:
        stdout.write_text(result.stdout, encoding="utf-8")


def function_addresses() -> dict[str, int]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        return {
            row["name"]: int(row["address"], 16)
            for row in csv.DictReader(handle)
        }


def compiled_symbols(obj: Path) -> dict[str, tuple[int, int]]:
    """Nesne dosyasindaki her fonksiyonun (ofset, boyut) bilgisi."""
    out = subprocess.run(
        ["arm-none-eabi-nm", "-S", str(obj)], capture_output=True, text=True
    ).stdout
    symbols = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in "tT":
            symbols[parts[3]] = (int(parts[0], 16), int(parts[1], 16))
    return symbols


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = [a for a in sys.argv[1:] if a.startswith("--")]
    if len(args) != 1:
        sys.exit(__doc__)
    source = Path(args[0])
    compiler = DEFAULT_CC
    for flag in flags:
        if flag.startswith("--cc="):
            compiler = flag.split("=", 1)[1]
    agbcc = AGBCC_DIR / compiler
    if not agbcc.exists():
        sys.exit(f"{compiler} kurulu degil. Once: make agbcc")
    if not ROM.exists():
        sys.exit("baserom.gba yok. Once: make prepare-rom ROM_ZIP=...")

    BUILD.mkdir(parents=True, exist_ok=True)
    stem = BUILD / source.stem
    run(["cpp", "-nostdinc", "-undef", str(source)], Path(f"{stem}.i"))
    run([str(agbcc), *CC1FLAGS, "-o", f"{stem}.s", f"{stem}.i"])
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.o", f"{stem}.s"])
    run(["arm-none-eabi-objcopy", "-O", "binary", f"{stem}.o", f"{stem}.bin"])

    rom = ROM.read_bytes()
    blob = Path(f"{stem}.bin").read_bytes()
    addresses = function_addresses()
    symbols = compiled_symbols(Path(f"{stem}.o"))

    print(f"{source}  [{compiler} {' '.join(CC1FLAGS)}]  {len(symbols)} fonksiyon")
    print(f"{'fonksiyon':22} {'boyut':>6}  sonuc")
    print("-" * 58)
    matched = []
    for name, (offset, size) in sorted(symbols.items(), key=lambda kv: kv[1][0]):
        address = addresses.get(name)
        if address is None:
            print(f"{name:22} {size:>6}  functions.csv'de adres yok")
            continue
        rom_offset = address - ROM_BASE
        mine = blob[offset:offset + size]
        theirs = rom[rom_offset:rom_offset + size]
        if mine == theirs:
            matched.append(name)
            print(f"{name:22} {size:>6}  BYTE-MATCHING  (0x{address:08X})")
        else:
            bad = sum(a != b for a, b in zip(mine, theirs))
            print(f"{name:22} {size:>6}  farkli: {bad}/{size} byte  (0x{address:08X})")
    print("-" * 58)
    print(f"{len(matched)}/{len(symbols)} fonksiyon C'den byte-matching")
    sys.exit(0 if len(matched) == len(symbols) else 1)


if __name__ == "__main__":
    main()
