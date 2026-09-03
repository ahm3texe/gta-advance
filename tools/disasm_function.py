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


def describe(value: int, names: dict[int, str]) -> str:
    """Bir literal havuzu kelimesinin ne oldugunu tahmin et."""
    if value in names:
        return f"{value:#010x}  {names[value]}"
    if 0x08000000 <= value < 0x0A000000:
        return f"{value:#010x}  ROM"
    if 0x02000000 <= value < 0x02040000:
        return f"{value:#010x}  EWRAM"
    if 0x03000000 <= value < 0x03008000:
        return f"{value:#010x}  IWRAM"
    if 0x04000000 <= value < 0x04000400:
        return f"{value:#010x}  I/O"
    if 0x05000000 <= value < 0x07000000:
        return f"{value:#010x}  palet/VRAM"
    return f"{value:#010x}  ({value if value < 0x80000000 else value - (1 << 32)})"


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

    rom_bytes = ROM.read_bytes()

    def word(addr: int) -> int | None:
        off = addr - ROM_BASE
        if 0 <= off + 4 <= len(rom_bytes):
            return int.from_bytes(rom_bytes[off:off + 4], "little")
        return None

    # Bilinen fonksiyon adlari: hem `bl` hedeflerini hem de havuzdaki
    # fonksiyon isaretcilerini isimlendirmek icin.
    names = {}
    for r in rows.values():
        addr = int(r["address"], 16)
        names[addr] = r["name"]
        names[addr | 1] = f"{r['name']}+Thumb"

    print(f"{target}  @ 0x{address:08X}  {size} byte  "
          f"[{'Thumb' if thumb else 'ARM'}]  — kaynak: baserom.gba")
    if row["notes"]:
        print(f"# {row['notes']}")
    print()
    ldr_pc = re.compile(r"ldr\s+r\d+, \[pc, #\d+\]\s*@ \(0x([0-9a-f]+)\)")
    branch = re.compile(r"\b(?:bl|b)\s+0x([0-9a-f]+)")
    for line in out.splitlines():
        if not re.match(r"\s*[0-9a-f]+:\s", line):
            continue
        m = ldr_pc.search(line)
        if m:
            value = word(int(m.group(1), 16))
            if value is not None:
                print(f"{line}   = {describe(value, names)}")
                continue
        m = branch.search(line)
        if m:
            hit = names.get(int(m.group(1), 16))
            if hit:
                print(f"{line}   <{hit}>")
                continue
        print(line)


if __name__ == "__main__":
    main()
