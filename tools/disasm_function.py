#!/usr/bin/env python3
"""ROM'daki bir fonksiyonun disassembly'sini uretir.

Kullanim:
    python3 tools/disasm_function.py EraseSaveSlot

Kaynak ROM'un kendisidir, depodaki bir dosya degil. Bu yuzden cikti her zaman
dogrudur ve bakim gerektirmez: bir fonksiyon C'ye tasindiktan sonra assembly
kaynagini silsen bile orijinal kodu istedigin an buradan geri alirsin.
"""
import csv
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
BUILD = ROOT / "build/disasm"
ROM_BASE = 0x08000000


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    target = sys.argv[1]

    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = {r["name"]: r for r in csv.DictReader(handle)}
    if target not in rows:
        sys.exit(f"{target} data/functions.csv icinde yok")

    row = rows[target]
    address = int(row["address"], 16)
    size = int(row["size"] or 0)
    if not size:
        sys.exit(f"{target} icin boyut bilinmiyor")

    BUILD.mkdir(parents=True, exist_ok=True)
    start = address - ROM_BASE
    slice_path = BUILD / f"{target}.bin"
    slice_path.write_bytes(ROM.read_bytes()[start:start + size])

    # ARM/Thumb ayrimi: notlarda ARM diye isaretlenmemisse Thumb varsayilir.
    thumb = "ARM" not in row["notes"].upper().split()
    out = subprocess.run([
        "arm-none-eabi-objdump", "-b", "binary", "-m", "arm7tdmi",
        "-M", "force-thumb" if thumb else "no-force-thumb",
        "-D", f"--adjust-vma={address:#x}", str(slice_path),
    ], capture_output=True, text=True).stdout

    print(f"{target}  @ 0x{address:08X}  {size} byte  "
          f"[{'Thumb' if thumb else 'ARM'}]  — kaynak: baserom.gba")
    if row["notes"]:
        print(f"# {row['notes']}")
    print()
    for line in out.splitlines():
        if re.match(r"\s*[0-9a-f]+:\s", line):
            print(line)


if __name__ == "__main__":
    main()
