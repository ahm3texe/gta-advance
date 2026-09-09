#!/usr/bin/env python3
"""Produce the disassembly of a function in the ROM.

Usage:
    python3 tools/disasm_function.py EraseSaveSlot

The source is the ROM itself, not a file in the repository. The output is
therefore always correct and needs no maintenance: even after a function moves
to C and its assembly source is deleted, the original code can be recovered
from here at any time.
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
    """Guess what a literal pool word is."""
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
    # A name or an address is accepted; address case does not matter.
    if target not in rows:
        want = target.lower()
        if want.startswith("0x"):
            byaddr = {r["address"].lower(): r for r in rows.values()}
            if want in byaddr:
                target = byaddr[want]["name"]
    if target not in rows:
        sys.exit(f"{target} is not in data/functions.csv")

    row = rows[target]
    address = int(row["address"], 16)
    size = int(row["size"] or 0)
    if not size:
        sys.exit(f"the size of {target} is unknown")

    BUILD.mkdir(parents=True, exist_ok=True)
    start = address - ROM_BASE
    slice_path = BUILD / f"{target}.bin"
    slice_path.write_bytes(ROM.read_bytes()[start:start + size])

    # ARM/Thumb distinction: Thumb is assumed unless the notes mark it as ARM.
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

    # Known function names: used to name both `bl` targets and function
    # pointers in the pool.
    names = {}
    for r in rows.values():
        addr = int(r["address"], 16)
        names[addr] = r["name"]
        names[addr | 1] = f"{r['name']}+Thumb"

    print(f"{target}  @ 0x{address:08X}  {size} byte  "
          f"[{'Thumb' if thumb else 'ARM'}]  - source: baserom.gba")
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
