#!/usr/bin/env python3
"""Find "conditional constant return" leaf functions in the ROM.

The pattern (a 12-byte core, followed by the pool + a second constant):
    ldr  rN, [pc, #imm]              <- base address from the pool
    ldr|ldrb|ldrh rD, [rN, #ofs]     <- read the field
    cmp  rD, #constant
    b<cond> FORWARD
    movs r0, #constant1              <- the fall-through path
    b    SON
    ...pool...
    movs r0, #constant2              <- the branch path
    bx   lr

The C equivalent is mechanical:
    u32 f(void) { if (symbol.field == CONST) return const2; return const1; }

Usage: python3 tools/find_predicates.py [--all]
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_BASE = 0x08000000
LOADS = [(0x6800, "u32", 4), (0x7800, "u8", 1), (0x8800, "u16", 2)]
CONDS = {0x0: "==", 0x1: "!=", 0xA: ">=", 0xB: "<", 0xC: ">", 0xD: "<="}


def main() -> None:
    rom = (ROOT / "baserom.gba").read_bytes()
    rows = list(csv.DictReader((ROOT / "data/functions.csv").open(newline="",
                                                                 encoding="utf-8")))
    ram = {int(r["address"], 16): r["name"]
           for r in csv.DictReader((ROOT / "data/ram_map.csv").open(newline="",
                                                                   encoding="utf-8"))}
    show_all = "--all" in sys.argv

    def hw(a):
        o = a - ROM_BASE
        return int.from_bytes(rom[o:o + 2], "little")

    def word(a):
        o = a - ROM_BASE
        return int.from_bytes(rom[o:o + 4], "little")

    found = []
    for row in rows:
        if not show_all and row["status"] == "matching":
            continue
        addr = int(row["address"], 16)
        if int(row["size"] or 0) < 16:
            continue
        h = [hw(addr + 2 * i) for i in range(6)]
        if (h[0] & 0xF800) != 0x4800:
            continue
        kind = scale = None
        for value, k, sc in LOADS:
            if (h[1] & 0xF800) == value:
                kind, scale = k, sc
                break
        if kind is None:
            continue
        if (h[2] & 0xF800) != 0x2800:                # cmp rD, #imm
            continue
        if (h[3] & 0xF000) != 0xD000:                # conditional branch
            continue
        cond = (h[3] >> 8) & 0xF
        if cond not in CONDS:
            continue
        if (h[4] & 0xFF00) != 0x2000:                # movs r0, #imm
            continue
        if (h[5] & 0xF800) != 0xE000:                # b
            continue
        pool = ((addr + 4) & ~3) + ((h[0] & 0xFF) * 4)
        sym = word(pool)
        offset = ((h[1] >> 6) & 0x1F) * scale
        want = h[2] & 0xFF
        fall = h[4] & 0xFF
        # the second constant at the branch target
        target = addr + 6 + 4 + ((h[3] & 0xFF) * 2)
        taken = hw(target) & 0xFF if (hw(target) & 0xFF00) == 0x2000 else None
        found.append((addr, sym, offset, kind, CONDS[cond], want, fall, taken,
                      ram.get(sym), row["name"]))

    print(f"{len(found)} conditional constant returns")
    for addr, sym, off, kind, cond, want, fall, taken, name, fname in found:
        label = name or "(not in ram_map)"
        t = taken if taken is not None else "?"
        print(f"0x{addr:08X}  {kind:3} 0x{sym:08X}+0x{off:02X} {cond} {want:<3} "
              f"-> {t}/{fall}  {label:22} {fname}")


if __name__ == "__main__":
    main()
