#!/usr/bin/env python3
import csv
import sys
from collections import Counter
from pathlib import Path


VALID_STATUSES = {"candidate", "discovered", "documented", "decompiled", "matching"}


def matching_region_summary(csv_path: Path) -> tuple[int, list[tuple[int, int]]]:
    if not csv_path.exists():
        return 0, []

    with csv_path.open(newline="", encoding="utf-8") as handle:
        intervals = sorted(
            (int(row["start"], 0), int(row["end"], 0))
            for row in csv.DictReader(handle)
        )

    merged: list[list[int]] = []
    for start, end in intervals:
        if merged and start <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])
    result = [(start, end) for start, end in merged]
    return sum(end - start for start, end in result), result


def c_source_summary(csv_path: Path) -> tuple[int, int, set[str]]:
    """(functions with a C source, those byte-matching from C, their addresses)."""
    if not csv_path.exists():
        return 0, 0, set()
    with csv_path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    matched = {r["address"].upper() for r in rows if r["matching"] == "yes"}
    return len(rows), len(matched), matched


def main() -> int:
    csv_path = Path(sys.argv[1] if len(sys.argv) > 1 else "data/functions.csv")
    with csv_path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))

    invalid = [row for row in rows if row["status"] not in VALID_STATUSES]
    if invalid:
        print(f"Error: {len(invalid)} rows have an invalid status.", file=sys.stderr)
        return 1

    counts = Counter(row["status"] for row in rows)
    total = len(rows)
    documented = counts["documented"] + counts["decompiled"] + counts["matching"]
    decompiled = counts["decompiled"] + counts["matching"]

    def percent(value: int) -> str:
        return f"{(100 * value / total):.2f}%" if total else "n/a"

    known_bytes = 0
    matching_bytes = 0
    for row in rows:
        raw_size = row["size"].strip()
        if not raw_size:
            continue
        size = int(raw_size, 0)
        known_bytes += size
        if row["status"] == "matching":
            matching_bytes += size

    print(f"Function map:             {total}")
    print(f"Reviewed functions:     {documented}/{total} ({percent(documented)})")
    print(f"With source:          {decompiled}/{total} ({percent(decompiled)})")
    print(f"Byte-matching:        {counts['matching']}/{total} ({percent(counts['matching'])})")
    if known_bytes:
        print(f"Matching code bytes:  {matching_bytes}/{known_bytes} ({100 * matching_bytes / known_bytes:.2f}%)")
    else:
        print("Matching code bytes:  n/a (function sizes are not known yet)")

    c_total, c_matched, c_addresses = c_source_summary(csv_path.parent / "c_sources.csv")
    if c_total:
        c_bytes = sum(
            int(row["size"], 0)
            for row in rows
            if row["address"].upper() in c_addresses and row["size"].strip()
        )
        print(f"With C source:        {c_total} functions, {c_matched} byte-matching")
        print(f"Matching bytes from C: {c_bytes}/{matching_bytes} "
              f"({100 * c_bytes / matching_bytes:.2f}% of matching)")

    libc_path = csv_path.parent / "libc_regions.csv"
    if libc_path.exists():
        with libc_path.open(newline="", encoding="utf-8") as handle:
            libc_bytes = sum(
                int(r["end"], 16) - int(r["address"], 16) for r in csv.DictReader(handle)
            )
        print(f"libc regions:         {libc_bytes} bytes "
              f"(verified against agbcc libc.a)")

    region_bytes, regions = matching_region_summary(csv_path.parent / "matching_regions.csv")
    if regions:
        print(f"Matching ROM region:  {region_bytes} unique bytes")
        start, end = max(regions, key=lambda interval: interval[1] - interval[0])
        print(f"Largest contiguous:   0x{start:08X}-0x{end - 1:08X} ({end - start} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
