#!/usr/bin/env python3
"""Report ROM bytes that no entry in data/functions.csv covers.

The function map was built from Ghidra's analysis plus the discovery passes, and
neither is complete: three separate gaps found by hand while decompiling turned
out to hold real functions (0x080673FC, 0x08067400 and 0x0805AB14, all reached
only through pointers or a single `bl`). This tool looks for the rest rather
than waiting to trip over them.

A gap is the space between one entry's end and the next entry's start. Most are
two bytes of alignment padding; those are reported separately and are not
interesting. What is interesting is a gap that holds instructions.

Each gap is classified:

  padding    every byte is zero, or it is a single 2-byte alignment slot
  stubs      it splits cleanly into complete `bx lr` bodies with nothing left
             over, so every function in it can be recovered exactly
  code       it holds instructions but does not split cleanly; the boundaries
             need a disassembler and a decision

`stubs` is the useful case: those bodies are short, their extents are certain,
and they can go straight into the map.

Usage:
    python3 tools/find_map_gaps.py                # summary and the stub gaps
    python3 tools/find_map_gaps.py --all          # every gap that holds code
    python3 tools/find_map_gaps.py --csv          # rows ready for functions.csv
"""

import argparse
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data" / "functions.csv"
ROM_BASE = 0x08000000

BX_LR = b"\x70\x47"             # bx lr
POP_PC = 0xBD00                 # pop {..., pc}

# A stub body is at most this long. Beyond it the split stops being obvious:
# a `bx lr` can sit in the middle of a function, after an early return, so a
# long run of them is not evidence of separate functions.
MAX_STUB = 8


def rom_bytes() -> bytes:
    if not ROM.exists():
        sys.exit(f"{ROM.name} is missing. First run: make prepare-rom")
    return ROM.read_bytes()


def entries() -> list[tuple[int, int, str]]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = [(int(r["address"], 16), int(r["size"]), r["name"])
                for r in csv.DictReader(handle)]
    return sorted(rows)


def gaps(rows: list[tuple[int, int, str]]) -> list[tuple[int, int, str, str]]:
    out = []
    for (a, size, name), (b, _, after) in zip(rows, rows[1:]):
        end = a + size
        if end < b:
            out.append((end, b - end, name, after))
    return out


def split_stubs(body: bytes, start: int) -> list[tuple[int, int]] | None:
    """Split a gap into complete bx-lr bodies, or return None if it does not.

    Trailing zero halfwords are alignment and are dropped; a zero halfword
    BETWEEN two bodies is the padding that follows an odd-length one.
    """
    found = []
    i = 0
    n = len(body)
    while i < n:
        if body[i:i + 2] == b"\x00\x00":        # padding between or after bodies
            i += 2
            continue
        j = i
        while j < n and body[j:j + 2] != BX_LR:
            j += 2
            if j - i > MAX_STUB:
                return None
        if j >= n:
            return None
        found.append((start + i, j + 2 - i))
        i = j + 2
    return found or None


def bl_targets(rom: bytes) -> dict[int, list[int]]:
    """Every Thumb `bl` target in the image, with the addresses that reach it."""
    out: dict[int, list[int]] = {}
    for i in range(0, len(rom) - 3, 2):
        hi = int.from_bytes(rom[i:i + 2], "little")
        lo = int.from_bytes(rom[i + 2:i + 4], "little")
        if (hi >> 11) == 0x1E and (lo >> 11) == 0x1F:
            off = ((hi & 0x7FF) << 12) | ((lo & 0x7FF) << 1)
            if off & 0x400000:
                off -= 0x800000
            out.setdefault(ROM_BASE + i + 4 + off, []).append(ROM_BASE + i)
    return out


def pool_words(rom: bytes) -> dict[int, list[int]]:
    """Every 4-aligned word that holds a ROM address."""
    out: dict[int, list[int]] = {}
    limit = ROM_BASE + len(rom)
    for i in range(0, len(rom) - 3, 4):
        value = int.from_bytes(rom[i:i + 4], "little")
        if ROM_BASE <= value < limit:
            out.setdefault(value, []).append(ROM_BASE + i)
    return out


def branches_into(rom: bytes, start: int, size: int, target: int) -> int:
    """Thumb b / b<cond> inside [start, start+size) that land on target."""
    hits = 0
    for i in range(start - ROM_BASE, start - ROM_BASE + size, 2):
        half = int.from_bytes(rom[i:i + 2], "little")
        if (half >> 11) == 0x1C:                       # b, 11-bit offset
            off = half & 0x7FF
            if off & 0x400:
                off -= 0x800
        elif (half >> 12) == 0xD and ((half >> 8) & 0xF) < 0xE:   # b<cond>
            off = half & 0xFF
            if off & 0x80:
                off -= 0x100
        else:
            continue
        if ROM_BASE + i + 4 + off * 2 == target:
            hits += 1
    return hits


def classify(body: bytes, start: int):
    if not any(body):
        return "padding", None
    if len(body) <= 2:
        return "padding", None
    pieces = split_stubs(body, start)
    if pieces:
        return "stubs", pieces
    return "code", None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--all", action="store_true",
                        help="list every gap that holds code, not just the stub ones")
    parser.add_argument("--csv", action="store_true",
                        help="print rows for the stub gaps, ready for functions.csv")
    args = parser.parse_args()

    rom = rom_bytes()
    rows = entries()
    found = gaps(rows)
    calls = bl_targets(rom)
    pool = pool_words(rom)

    def evidence(addr: int) -> str:
        parts = []
        if calls.get(addr):
            parts.append(f"bl x{len(calls[addr])}")
        if pool.get(addr | 1):
            parts.append(f"ptr x{len(pool[addr | 1])}")
        if pool.get(addr):
            parts.append(f"word x{len(pool[addr])}")
        return ", ".join(parts) if parts else "none"

    def is_tail(addr: int) -> bool:
        prev = None
        for a, size, _ in rows:
            if a < addr:
                prev = (a, size)
            else:
                break
        return bool(prev) and branches_into(rom, prev[0], prev[1], addr) > 0

    buckets = {"padding": [], "stubs": [], "code": []}
    stub_rows = []
    for start, length, before, after in found:
        body = rom[start - ROM_BASE:start - ROM_BASE + length]
        kind, pieces = classify(body, start)
        buckets[kind].append((start, length, before, after, body, pieces))
        if kind == "stubs":
            stub_rows.extend(pieces)

    if args.csv:
        print("address,name,size,status,module,notes")
        for addr, size in stub_rows:
            print(f'0x{addr:08X},FUN_{addr:08x},{size},candidate,unknown,'
                  f'"found by tools/find_map_gaps.py in a gap no map entry covered"')
        return

    total = sum(b[1] for b in buckets["code"]) + sum(b[1] for b in buckets["stubs"])
    print(f"gaps: {len(found)}  ({sum(b[1] for b in found)} bytes)")
    print(f"  padding : {len(buckets['padding']):4d}  "
          f"{sum(b[1] for b in buckets['padding']):6d} bytes")
    print(f"  stubs   : {len(buckets['stubs']):4d}  "
          f"{sum(b[1] for b in buckets['stubs']):6d} bytes  "
          f"-> {len(stub_rows)} functions")
    print(f"  code    : {len(buckets['code']):4d}  "
          f"{sum(b[1] for b in buckets['code']):6d} bytes")
    print(f"  unmapped code in total: {total} bytes")

    referenced = [p for _, _, _, _, _, pieces in buckets["stubs"]
                  for p in pieces if evidence(p[0]) != "none"]
    tails = [p for _, _, _, _, _, pieces in buckets["stubs"]
             for p in pieces if is_tail(p[0])]
    print(f"  of the {len(stub_rows)} stub bodies: {len(referenced)} are referenced, "
          f"{len(tails)} are branched into by the entry before them")

    print("\nstub gaps (extents certain):")
    for start, length, before, after, body, pieces in buckets["stubs"]:
        for addr, size in pieces:
            mark = "TAIL" if is_tail(addr) else evidence(addr)
            print(f"  {addr:#010x} /{size:<3d} {mark:<24s} [{before} | {after}]")

    reported = sum(size for _, size, _ in rows)
    honest = reported + total
    print(f"\ndenominator: the map sums to {reported} bytes; with the {total} bytes "
          f"of unmapped code that is {honest}.")
    print(f"  a figure of X/{reported} is therefore high by a factor of "
          f"{honest / reported:.4f} against X/{honest}.")

    if args.all:
        print("\ngaps holding code (boundaries need a decision):")
        for start, length, before, after, body, _ in buckets["code"]:
            print(f"  {start:#010x} +{length:<5d} {body[:12].hex()}...  "
                  f"[{before} | {after}]")


if __name__ == "__main__":
    main()
