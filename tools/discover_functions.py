#!/usr/bin/env python3
"""Search the gaps in the function map for functions Ghidra missed.

data/functions.csv comes from Ghidra's automatic analysis and was proven
incomplete for this ROM: five functions were found by hand while resolving
handler pointers in the pools (0x0803DEA8, 0x0803E280, 0x0803F0E8,
0x080397EC...). This tool does the same job systematically.

THREE METHODS:

A) CALL TARGET (strong). The target of a `bl` inside known code is a function
   entry. Two things have to be excluded first, and the second was learned the
   hard way:

   1. LITERAL POOLS. A pool word can decode as a `bl` pair. 0x0800E3C8 holds
      the constant 0xFFFFF07F, read by the `ldr rX,[pc,#imm]` at 0x0800E386,
      and decodes as `bl 0x0808E3CA` -- an address 0x1C000 past the last real
      function, in graphics data. This tool once added it as "certain". Every
      pc-relative load target inside a function is now marked and skipped.
   2. THE CODE LIMIT. A target past the end of the last mapped function is not
      a call; method B already applied that bound and method A now does too.

   No prologue pattern is needed -- leaf functions are found too.

B) FUNCTION POINTER (strong). Jump tables and handler arrays in the ROM data
   hold Thumb pointers: 0x08xxxxxx, ODD. A raw scan gives many false positives
   (coincidences inside graphics data), so the target is required to have a
   REAL prologue.

C) PROLOGUE PATTERN (probable). Searches the gaps for a Thumb prologue
   (`push {..., lr}`), then extracts the body with the audit_boundaries walker.
   Four strict conditions:
   1. the body is 8-4096 bytes with no unresolved indirect jump
   2. the body does not overflow the gap (does not enter a known function)
   3. it is preceded by a terminator (`bx lr` / `pop {..,pc}` / `bx rN`) or by
      alignment padding -- i.e. it really is at a function boundary
   4. of candidates containing one another, only the outer one is taken

What is found is added with the `discovered` status: the address and boundary
are automatic, and the bodies have not been reviewed yet.

Usage:
    python3 tools/discover_functions.py            # report only
    python3 tools/discover_functions.py --apply    # add to functions.csv
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_boundaries import Walker, in_arm_range, ARM_FUNCTIONS  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
NON_FUNCTIONS = ROOT / "data/non_function_entries.csv"
ROM_BASE = 0x08000000
MIN_SIZE, MAX_SIZE = 8, 4096


def main() -> None:
    apply = "--apply" in sys.argv
    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fields = list(rows[0].keys())
    rejected = {
        int(row["address"], 16)
        for row in csv.DictReader(NON_FUNCTIONS.open(newline="", encoding="utf-8"))
    } if NON_FUNCTIONS.exists() else set()

    known = sorted((int(r["address"], 16), int(r["size"] or 0)) for r in rows
                   if r["size"].strip())
    gaps, prev = [], None
    for address, size in known:
        if prev is not None and address > prev:
            gaps.append((prev, address))
        prev = max(prev or 0, address + size)

    def half(addr: int) -> int:
        off = addr - ROM_BASE
        return int.from_bytes(rom[off:off + 2], "little")

    covered = set()
    for address, size in known:
        covered.update(range(address, address + size))
    known_starts = {address for address, _ in known}

    # A) Call targets. Strong, but not free of false positives: see the header.
    code_limit = max(address + size for address, size in known)
    call_targets = set()
    for address, size in known:
        base = address - ROM_BASE
        # Every word this function loads pc-relatively is a literal pool entry.
        # Those four bytes are DATA and must not be decoded as instructions.
        pool = set()
        for i in range(0, max(0, size - 1), 2):
            half_word = int.from_bytes(rom[base + i:base + i + 2], "little")
            if (half_word & 0xF800) == 0x4800:      # ldr rX,[pc,#imm]
                word = (((address + i + 4) & ~3) + (half_word & 0xFF) * 4)
                pool.update(range(word, word + 4))
        for i in range(0, max(0, size - 2), 2):
            if address + i in pool or address + i + 2 in pool:
                continue
            hw1 = int.from_bytes(rom[base + i:base + i + 2], "little")
            hw2 = int.from_bytes(rom[base + i + 2:base + i + 4], "little")
            if (hw1 & 0xF800) == 0xF000 and (hw2 & 0xF800) == 0xF800:
                offset = hw1 & 0x7FF
                if offset & 0x400:
                    offset -= 0x800
                dest = address + i + 4 + (offset << 12) + ((hw2 & 0x7FF) << 1)
                if ROM_BASE <= dest < code_limit:
                    call_targets.add(dest)

    from_calls = []
    inside_known = []
    for target in sorted(call_targets):
        if target in rejected:
            continue
        if target in known_starts:
            continue
        if target in covered:
            inside_known.append(target)      # a sign of a boundary error
            continue
        if in_arm_range(target) or target in ARM_FUNCTIONS:
            continue
        walker = Walker(rom, target)
        walker.run()
        size = walker.code_extent()
        if MIN_SIZE <= size <= MAX_SIZE and not walker.unresolved:
            from_calls.append((target, size))

    # B) Function pointers: only when the target has a real prologue.
    limit = code_limit
    from_pointers = []
    seen_ptr = set()
    for off in range(0, len(rom) - 4, 4):
        word = int.from_bytes(rom[off:off + 4], "little")
        if not (ROM_BASE <= word < limit and (word & 1)):
            continue
        target = word & ~1
        if (target in known_starts or target in covered or target in seen_ptr
                or target in rejected):
            continue
        if in_arm_range(target) or target in ARM_FUNCTIONS:
            continue
        head = int.from_bytes(rom[target - ROM_BASE:target - ROM_BASE + 2], "little")
        if not ((head & 0xFF00) == 0xB500 or (head & 0xFE00) == 0xB400):
            continue                     # without a real prologue, treat as coincidence
        seen_ptr.add(target)
        walker = Walker(rom, target)
        walker.run()
        size = walker.code_extent()
        if MIN_SIZE <= size <= MAX_SIZE and not walker.unresolved:
            from_pointers.append((target, size))

    found = list(from_calls) + list(from_pointers)
    for gap_start, gap_end in gaps:
        addr = (gap_start + 1) & ~1
        while addr + 2 <= gap_end:
            word = half(addr)
            if (word & 0xFF00) == 0xB500 and half(addr + 2) not in (0x0000, 0xFFFF):
                if (not in_arm_range(addr) and addr not in ARM_FUNCTIONS
                        and addr not in rejected):
                    walker = Walker(rom, addr)
                    walker.run()
                    size = walker.code_extent()
                    prev_word = half(addr - 2) if addr - 2 >= gap_start else 0x4770
                    terminated = (prev_word == 0x4770
                                  or (prev_word & 0xFF00) == 0xBD00
                                  or (prev_word & 0xFF87) == 0x4700
                                  or prev_word == 0x0000)
                    if (MIN_SIZE <= size <= MAX_SIZE and not walker.unresolved
                            and addr + size <= gap_end and terminated):
                        found.append((addr, size))
            addr += 2

    # Filter out containment: a prologue pattern inside a function's body is
    # not a separate function.
    found.sort()
    unique, last_end = [], 0
    for addr, size in found:
        if addr < last_end:
            continue
        unique.append((addr, size))
        last_end = addr + size

    total = sum(size for _, size in unique)
    print(f"Gaps: {len(gaps)}")
    print(f"A) Certain, from call targets:  {len(from_calls)} functions")
    print(f"B) From pointers (with prologue): {len(from_pointers)} functions")
    print(f"C) Probable, from prologue pattern: "
          f"{len(unique) - len(from_calls) - len(from_pointers)} functions")
    print(f"Total: {len(unique)} functions, {total} bytes")
    if rejected:
        print(f"Manually rejected false entries: {len(rejected)} "
              "(data/non_function_entries.csv)")
    if inside_known:
        print(f"\nWARNING: {len(inside_known)} `bl` targets fall INSIDE a known "
              f"function -- those functions' boundaries may be wrong.")
        print("  Example: " + ", ".join(f"0x{t:08X}" for t in inside_known[:6]))
    print()
    print(f"{'address':12} {'bytes':>6}")
    print("-" * 20)
    for addr, size in sorted(unique, key=lambda f: -f[1])[:15]:
        print(f"0x{addr:08X} {size:>6}")

    if not apply:
        print("\n(report only; use --apply to add them)")
        return

    for addr, size in unique:
        rows.append({
            "address": f"0x{addr:08X}", "name": f"FUN_{addr:08x}",
            "size": str(size), "status": "discovered", "module": "unknown",
            "notes": ("missed by Ghidra; tools/discover_functions.py "
                      + ("call target (certain)" if (addr, size) in from_calls
                         else "function pointer" if (addr, size) in from_pointers
                         else "prologue pattern (probable)")
                      + ", body not yet reviewed"),
        })
    rows.sort(key=lambda r: int(r["address"], 16))
    with FUNCTIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nfunctions.csv: {len(unique)} new functions added.")


if __name__ == "__main__":
    main()
