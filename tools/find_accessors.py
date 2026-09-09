#!/usr/bin/env python3
"""Find simple leaf accessors in the ROM (both reads AND writes).

The pattern (a 6-byte core, followed by alignment + pool):
    ldr rN, [pc, #imm]      <- base address from the pool
    ldr|ldrb|ldrh rD, [rN, #ofs]      (getter)
      or
    str|strb|strh rD, [rN, #ofs]      (setter; the value arrives in r0, so the
                                       base is loaded into r1 because r0 is taken)
    bx lr

The C equivalent is mechanical:
    u32  f(void)      { return symbol.field; }
    void f(u32 value) { symbol.field = value; }

Usage: python3 tools/find_accessors.py [--all]
  default: only records that are NOT YET MATCHING
  --all: also shows matching ones, for verification
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_BASE = 0x08000000

# (mask, value, kind, scale) -- offset scaling comes from the Thumb encoding
ACCESS = [
    # (mask, value, kind, scale, direction)
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
    print(f"{len(found)} leaf accessors ({gets} getters, {len(found) - gets} setters)")
    for addr, sym, off, kind, direction, name, fname in found:
        label = name or "(not in ram_map)"
        print(f"0x{addr:08X}  {direction:3} {kind:3} 0x{sym:08X}+0x{off:02X}  "
              f"{label:22} {fname}")


if __name__ == "__main__":
    main()
