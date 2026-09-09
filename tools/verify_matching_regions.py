#!/usr/bin/env python3
"""Rebuild every region in data/matching_regions.csv and verify it against the ROM."""

import csv
import sys
from pathlib import Path


ROM_BASE = 0x08000000


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom_path = root / "baserom.gba"
    csv_path = root / "data/matching_regions.csv"
    rom = rom_path.read_bytes()

    with csv_path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))

    intervals: list[tuple[int, int]] = []
    for row in rows:
        start = int(row["start"], 0)
        end = int(row["end"], 0)
        binary_path = root / row["binary"]
        generated = binary_path.read_bytes()
        expected_size = end - start
        offset = start - ROM_BASE

        if expected_size <= 0:
            print(f"ERROR: invalid region {row['start']}-{row['end']}", file=sys.stderr)
            return 1
        if len(generated) != expected_size:
            print(
                f"ERROR: {binary_path} is {len(generated)} bytes; expected {expected_size}",
                file=sys.stderr,
            )
            return 1
        if generated != rom[offset:offset + expected_size]:
            print(f"ERROR: {binary_path} does not match the ROM", file=sys.stderr)
            return 1
        intervals.append((start, end))

    merged: list[list[int]] = []
    for start, end in sorted(intervals):
        if merged and start <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])

    total = sum(end - start for start, end in merged)
    print(f"MATCHING REGIONS: {len(rows)} fragments, {total} unique ROM bytes")
    for start, end in merged:
        print(f"  0x{start:08X}–0x{end - 1:08X}: {end - start} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

