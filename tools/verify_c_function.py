#!/usr/bin/env python3
"""Bir C dosyasini agbcc ile derler, linkler ve her fonksiyonu ROM ile karsilastirir.

Kullanim:  python3 tools/verify_c_function.py src/save/save_helpers.c [--cc=agbcc]

Fonksiyon adresleri data/functions.csv'den okunur. Bir fonksiyon C'den
byte-matching oldugunda, esdeger assembly kaynagi artik gereksizdir.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import (  # noqa: E402
    CC1FLAGS, DEFAULT_CC, ROM_BASE, compile_and_link, function_rows, rom_bytes,
)


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    compiler = next(
        (a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--cc=")),
        DEFAULT_CC,
    )
    if len(args) != 1:
        sys.exit(__doc__)
    source = Path(args[0])

    blob, layout, base = compile_and_link(source, compiler)
    rom = rom_bytes()
    rows = function_rows()

    print(f"{source}  [{compiler} {' '.join(CC1FLAGS)}]  "
          f"{len(layout)} fonksiyon @ 0x{base:08X}")
    print(f"{'fonksiyon':22} {'boyut':>6}  sonuc")
    print("-" * 60)
    matched = 0
    for name, (offset, size) in sorted(layout.items(), key=lambda kv: kv[1][0]):
        address = int(rows[name]["address"], 16)
        mapped_size = int(rows[name]["size"], 0)
        mine = blob[offset:offset + size]
        theirs = rom[address - ROM_BASE:address - ROM_BASE + size]
        if size < mapped_size:
            print(f"{name:22} {size:>6}  KISA C CIKTISI; harita {mapped_size} byte "
                  f"(0x{address:08X})")
        elif mine == theirs:
            matched += 1
            print(f"{name:22} {size:>6}  BYTE-MATCHING  (0x{address:08X})")
        else:
            bad = sum(a != b for a, b in zip(mine, theirs))
            print(f"{name:22} {size:>6}  farkli: {bad}/{size} byte  (0x{address:08X})")
    print("-" * 60)
    print(f"{matched}/{len(layout)} fonksiyon C'den byte-matching")
    sys.exit(0 if matched == len(layout) else 1)


if __name__ == "__main__":
    main()
