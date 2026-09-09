#!/usr/bin/env python3
"""Check the function boundaries in data/functions.csv against the ROM.

Ghidra fails to resolve jump tables and cuts functions in half. A wrong
boundary means wasted effort at every stage: while trying to write one
function you are in fact writing a fragment of one.

METHOD: recursive descent. Starting from the entry point, only instructions
that are ACTUALLY REACHED are decoded. Linear disassembly cannot be used,
because literal pools and data bytes decode as instructions and produce false
branches; in this repository that approach once "grew" 998 functions to 8 KB.

Literal pool addresses are marked as DATA and never decoded. Jump tables are
found by looking backwards from `mov pc, rN`, and entries are followed only
while they stay in a plausible range.

Usage:
    python3 tools/audit_boundaries.py            # report + self-test
    python3 tools/audit_boundaries.py --check-baseline
    python3 tools/audit_boundaries.py --write-baseline
    python3 tools/audit_boundaries.py --apply    # correct functions.csv
"""
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
REGIONS = ROOT / "data/matching_regions.csv"
BASELINE = ROOT / "data/boundary_baseline.json"
ARM_REVIEW = ROOT / "data/arm_boundary_review.csv"
ROM_BASE = 0x08000000
# The game has real Thumb functions larger than 8 KiB (0x0802BDF0: 8320 B).
# The walker's ceiling is kept high enough not to cut a real body in half.
MAX_EXTENT = 16384

# Permanent ARM functions: the walker decodes Thumb, so these are skipped.
ARM_FUNCTIONS = {0x080000C0, 0x08000104}

# The ARM code region. Measured: ONE contiguous range, NOT four separate
# pieces. [0x08067E04, 0x0806B84C) = 14920 bytes. Evidence: in ALL 3730 words
# of the range the condition field is != 0xF (random data/Thumb would show 0xF
# in about 1 word in 16); immediately before it the ratio is 0.9533 and after
# it 0.9747. The upper bound 0x0806B84C is the start of the BIOS swi thunks
# (Thumb).
#
# The previous four-range constant contained no false positives but was
# missing 6524 bytes (44%); all four missed pieces also give a ratio of 1.0000.
#
# Contents: NOT an audio driver (there is no m4a/sappy signature in the ROM
# and not a single audio register access in this region) -- affine texture
# mapping, Cohen-Sutherland clipping and 8bpp tiled framebuffer addressing,
# i.e. a rasterizer.
ARM_RANGES = [
    (0x08067E04, 0x0806B84C),
]

# GROWTH beyond the recorded boundary by more than this counts as an analysis
# error. Real functions whose total size exceeds 4 KiB are not suspect on their
# own.
SANE_GROWTH = 4096


def build_snapshot(grown, skipped_arm, skipped_jump, skipped_big, conflicts):
    """An ordered, machine-comparable view of the map debt."""

    def measured(items):
        return [
            {
                "address": row["address"],
                "recordedSize": old,
                "reachableSize": new,
            }
            for row, old, new in sorted(items, key=lambda item: int(item[0]["address"], 16))
        ]

    return {
        "schemaVersion": 1,
        "shortBoundaries": [
            {
                "address": row["address"],
                "recordedSize": old,
                "reachableSize": new,
                "swallows": [f"0x{address:08X}" for address in eaten],
            }
            for row, old, new, eaten in sorted(
                grown, key=lambda item: int(item[0]["address"], 16)
            )
        ],
        "skippedArm": sorted(row["address"] for row in skipped_arm),
        "skippedUnresolved": measured(skipped_jump),
        "skippedOversized": measured(skipped_big),
        "matchingConflicts": measured(conflicts),
    }


def compare_baseline(snapshot: dict) -> None:
    if not BASELINE.exists():
        sys.exit(f"ERROR: {BASELINE.relative_to(ROOT)} is missing; run --write-baseline first")
    expected = json.loads(BASELINE.read_text(encoding="utf-8"))
    if snapshot == expected:
        print(f"Boundary baseline: CLEAN ({len(snapshot['shortBoundaries'])} known findings)")
        return

    expected_addresses = {row["address"] for row in expected.get("shortBoundaries", [])}
    actual_addresses = {row["address"] for row in snapshot["shortBoundaries"]}
    added = sorted(actual_addresses - expected_addresses)
    removed = sorted(expected_addresses - actual_addresses)
    print("ERROR: the boundary audit diverged from the baseline.", file=sys.stderr)
    if added:
        print(f"  new: {', '.join(added)}", file=sys.stderr)
    if removed:
        print(f"  closed: {', '.join(removed)}", file=sys.stderr)
    if not added and not removed:
        print("  same addresses; a size/swallowed record or a skipped group changed.", file=sys.stderr)
    print("If the change is correct, review it and run `make boundary-baseline`.", file=sys.stderr)
    sys.exit(1)


def in_arm_range(addr: int) -> bool:
    return any(lo <= addr < hi for lo, hi in ARM_RANGES)


class Walker:
    """Walk the reachable instructions of a single function."""

    def __init__(self, rom: bytes, start: int):
        self.rom = rom
        self.start = start
        self.code = set()       # addresses of decoded instructions
        self.data = set()       # literal pool words
        self.unresolved = False  # is there an unresolved indirect jump

    def hw(self, addr: int) -> int:
        off = addr - ROM_BASE
        return int.from_bytes(self.rom[off:off + 2], "little")

    def word(self, addr: int) -> int:
        off = addr - ROM_BASE
        return int.from_bytes(self.rom[off:off + 4], "little")

    def in_range(self, addr: int) -> bool:
        return self.start <= addr < self.start + MAX_EXTENT

    def jump_table(self, dispatch: int) -> list[int]:
        """Read table entries from the pool load preceding `mov pc, rN`.

        Pattern: ldr rX, [pc, #N] -> table base; lsl/add; ldr; mov pc.
        The table is read for as long as in-range even addresses continue.
        """
        base = None
        for back in range(2, 20, 2):
            addr = dispatch - back
            if addr < self.start:
                break
            h = self.hw(addr)
            if (h & 0xF800) == 0x4800:                      # ldr rX, [pc, #imm]
                pool = ((addr + 4) & ~3) + ((h & 0xFF) * 4)
                candidate = self.word(pool)
                if self.in_range(candidate):
                    base = candidate
                    self.data.update(range(pool, pool + 4))
                    break
        if base is None:
            return []
        targets = []
        for i in range(256):
            entry = base + 4 * i
            value = self.word(entry)
            if value % 2 or not self.in_range(value) or value < self.start:
                break
            targets.append(value)
            self.data.update(range(entry, entry + 4))
        return targets

    def run(self) -> None:
        pending = [self.start]
        while pending:
            addr = pending.pop()
            while True:
                if addr in self.code or not self.in_range(addr):
                    break
                if addr in self.data:
                    break
                self.code.add(addr)
                h = self.hw(addr)
                nxt = addr + 2

                if (h & 0xF800) == 0xF000:                  # 32-bit bl/blx
                    self.code.add(addr + 2)
                    addr += 4
                    continue
                if (h & 0xF800) == 0x4800:                  # ldr rX, [pc, #imm]
                    pool = ((addr + 4) & ~3) + ((h & 0xFF) * 4)
                    if self.in_range(pool):
                        self.data.update(range(pool, pool + 4))
                    addr = nxt
                    continue
                if (h & 0xF000) == 0xD000:                  # conditional branch / swi
                    cond = (h >> 8) & 0xF
                    if cond < 0xE:
                        off = h & 0xFF
                        if off & 0x80:
                            off -= 0x100
                        pending.append(addr + 4 + off * 2)
                    addr = nxt
                    continue
                if (h & 0xF800) == 0xE000:                  # unconditional branch
                    off = h & 0x7FF
                    if off & 0x400:
                        off -= 0x800
                    pending.append(addr + 4 + off * 2)
                    break
                if h == 0x4770:                             # bx lr
                    break
                if (h & 0xFF00) == 0xBD00:                  # pop {..., pc}
                    break
                if (h & 0xFF87) == 0x4700:                  # bx rN
                    break
                if (h & 0xFF87) == 0x4687:                  # mov pc, rN
                    targets = self.jump_table(addr)
                    if targets:
                        pending.extend(targets)
                    else:
                        self.unresolved = True
                    break
                addr = nxt

    def code_extent(self) -> int:
        """The length covered by the reached INSTRUCTIONS only.

        The `size` in the CSV is the body length and does not include the
        literal pool; a real boundary error exists only once code exceeds the
        body.
        """
        return (max(self.code) + 2 - self.start) if self.code else 0

    def full_extent(self) -> int:
        """Code + literal pool: the measure matching_regions.csv uses."""
        covered = self.code | self.data
        return (max(covered) + 4 - self.start) if covered else 0

    def tail_pool_extent(self, limit: int) -> int:
        """The code body + ONLY its own pool immediately after the body.

        full_extent() reached too far: the walker follows tail calls and also
        collects OTHER functions' pools, so 1651 records were flagged and 37 of
        them grew over a verified record. Here only words satisfying all of the
        following are added:
          - CONTIGUOUS from where the code body ENDS,
          - referenced by this function's own ldr (self.data),
          - without exceeding `limit` (the start of the next record).
        The first gap breaks the chain; nothing beyond the pool is taken.
        """
        if not self.code:
            return 0
        end = max(self.code) + 2
        # Alignment padding belongs to the function ONLY if a real pool word
        # follows it. Rounding unconditionally measured 41 functions without a
        # pool exactly +2 bytes too long (NoOpVBlankFinalize is really 2 bytes,
        # the tool said 4).
        aligned = (end + 3) & ~3
        if aligned + 4 <= limit and aligned in self.data:
            end = aligned
            while end + 4 <= limit and end in self.data:
                end += 4
        return end - self.start


def main() -> None:
    apply = "--apply" in sys.argv
    check_baseline = "--check-baseline" in sys.argv
    write_baseline = "--write-baseline" in sys.argv
    if sum((apply, check_baseline, write_baseline)) > 1:
        sys.exit("--apply, --check-baseline and --write-baseline cannot be combined")
    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fields = list(rows[0].keys())
    reviewed_arm = {
        int(row["address"], 16)
        for row in csv.DictReader(ARM_REVIEW.open(newline="", encoding="utf-8"))
    } if ARM_REVIEW.exists() else set()

    # Self-test. The correct invariant: the REACHABLE CODE of a byte-matching
    # function cannot extend outside the verified region containing it.
    # (The `size` field in functions.csv cannot be used for this: in some
    # matching functions that field is stale, e.g. VBlankIntr says 312 while the
    # body, together with its inline pools, reaches the end of the region.)
    regions = [(int(r["start"], 16), int(r["end"], 16))
               for r in csv.DictReader(REGIONS.open(newline="", encoding="utf-8"))]

    def containing(addr: int):
        for lo, hi in regions:
            if lo <= addr < hi:
                return lo, hi
        return None

    print("Self-test (byte-matching functions):")
    bad = checked = 0
    for row in rows:
        if row["status"] != "matching":
            continue
        start = int(row["address"], 16)
        if start in ARM_FUNCTIONS:
            continue
        region = containing(start)
        if region is None:
            continue
        checked += 1
        w = Walker(rom, start)
        w.run()
        if w.code and max(w.code) + 2 > region[1]:
            bad += 1
            if bad <= 5:
                print(f"  OVERFLOW {row['address']} {row['name']}: "
                      f"region 0x{region[1]:08X}, reached code "
                      f"0x{max(w.code) + 2:08X}")
    print(f"  {checked} functions tested, {bad} overflows\n")
    if bad:
        print("The tool overflows on verified functions; --apply is not safe.")
        if apply:
            sys.exit(1)

    known = sorted(int(r["address"], 16) for r in rows)
    verified = {int(r["address"], 16) for r in rows if r["status"] == "matching"}
    grown, swallowed = [], {}
    skipped_arm, skipped_jump, skipped_big, conflicts = [], [], [], []
    for row in rows:
        if not row["size"].strip() or row["status"] == "matching":
            continue
        start, size = int(row["address"], 16), int(row["size"])
        if size <= 0:
            continue
        if start in ARM_FUNCTIONS or in_arm_range(start):
            if start not in reviewed_arm:
                skipped_arm.append(row)
            continue
        w = Walker(rom, start)
        w.run()
        nxt = next((a for a in known if a > start), start + MAX_EXTENT)
        got = w.tail_pool_extent(nxt)
        if got <= size:
            continue
        # Conservative gate: with an unresolved indirect jump the walker may
        # not have seen part of the body, and implausible growth points to a
        # mode error. Neither is corrected automatically.
        if w.unresolved:
            skipped_jump.append((row, size, got))
            continue
        if got - size > SANE_GROWTH:
            skipped_big.append((row, size, got))
            continue
        eaten = [a for a in known if start < a < start + got]
        # Safety gate: growth over a verified byte-matching record is not
        # accepted. Such a conflict indicates the tool is wrong; it is reported
        # for manual review.
        if any(a in verified for a in eaten):
            conflicts.append((row, size, got))
            continue
        grown.append((row, size, got, eaten))
        for a in eaten:
            swallowed[a] = start

    print(f"{len(grown)} functions have a short boundary; "
          f"{len(swallowed)} records fall inside another function.")
    print(f"Left out of the audit (not silently dropped; these need manual review):")
    print(f"  {len(skipped_arm):4d} in the ARM range")
    print(f"  {len(skipped_jump):4d} contain an unresolved indirect jump")
    print(f"  {len(skipped_big):4d} implausible extra growth (>{SANE_GROWTH} bytes), "
          f"possibly a mode error")
    print(f"  {len(conflicts):4d} grow over a verified function")
    print()
    print(f"{'address':12} {'old':>6} {'new':>6}  swallowed")
    print("-" * 58)
    for row, old, new, eaten in sorted(grown, key=lambda g: g[2] - g[1],
                                       reverse=True)[:20]:
        tag = " ".join(f"0x{a:08X}" for a in eaten[:3])
        if len(eaten) > 3:
            tag += f" (+{len(eaten) - 3})"
        print(f"{row['address']} {old:>6} {new:>6}  {tag}")

    snapshot = build_snapshot(grown, skipped_arm, skipped_jump, skipped_big, conflicts)
    if write_baseline:
        BASELINE.write_text(
            json.dumps(snapshot, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"\nBoundary baseline written: {BASELINE.relative_to(ROOT)}")
        return
    if check_baseline:
        if bad:
            sys.exit("ERROR: a byte-matching function overflow cannot be accepted by the baseline")
        print()
        compare_baseline(snapshot)
        return

    if not apply:
        print("\n(report only; use --apply to change files)")
        return

    out = []
    for row in rows:
        if int(row["address"], 16) in swallowed:
            continue
        for grown_row, _old, new, _eaten in grown:
            if grown_row is row:
                row["size"] = str(new)
                note = row["notes"].strip('"')
                row["notes"] = (note + "; boundary corrected against the ROM "
                                "(audit_boundaries.py)").lstrip("; ")
                break
        out.append(row)

    with FUNCTIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(out)
    print(f"\nfunctions.csv updated: {len(grown)} sizes corrected, "
          f"{len(swallowed)} records deleted.")


if __name__ == "__main__":
    main()
