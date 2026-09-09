#!/usr/bin/env python3
"""Rename a function or RAM symbol across the WHOLE repository in one operation.

WHY IT EXISTS
-------------
Naming broke the same way three times: the name in `data/functions.csv` was
changed but the C sources using the old name were not updated. The result was
the same every time -- `agbcc_build.py` could not resolve the symbol, and the
error only surfaced minutes later during `make check`, piled on top of other
work.

This tool changes the name not in one place but EVERYWHERE it occurs: data
tables, C sources, headers and documents. It first verifies that the address
really carries that name, then checks that the new name is not already used by
something else; if either fails, nothing is written.

USAGE
-----
  python3 tools/rename_symbol.py 0x0803C400 GetOwnerSlot
  python3 tools/rename_symbol.py --dry-run 0x08066A54 ResetLinkHardware
"""

import argparse
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLES = ["data/functions.csv", "data/function_overrides.csv", "data/ram_map.csv"]
TEXT_DIRS = ["src", "include", "docs", "data"]


def load(path):
    with (ROOT / path).open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    return rows


def save(path, rows):
    with (ROOT / path).open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("address", help="0x08066A54 gibi")
    parser.add_argument("new_name")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    address = args.address.upper().replace("X", "x")
    new_name = args.new_name

    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", new_name):
        sys.exit(f"invalid name: {new_name!r}")

    old_name = None
    for table in TABLES:
        for row in load(table):
            if row["address"].upper().replace("X", "x") == address:
                old_name = row["name"]
                break
        if old_name:
            break
    if old_name is None:
        sys.exit(f"{address} is in no data table")
    if old_name == new_name:
        sys.exit(f"{address} zaten {new_name}")

    # Stop if the new name is used at another address: a silent clash is the worst case.
    for table in TABLES:
        for row in load(table):
            if (row["name"] == new_name
                    and row["address"].upper().replace("X", "x") != address):
                sys.exit(f"{new_name} is already used by {row['address']}")

    pattern = re.compile(rf"\b{re.escape(old_name)}\b")
    touched = []

    for table in TABLES:
        rows = load(table)
        changed = False
        for row in rows:
            for key, value in row.items():
                if value and pattern.search(value):
                    row[key] = pattern.sub(new_name, value)
                    changed = True
        if changed:
            touched.append(table)
            if not args.dry_run:
                save(table, rows)

    for directory in TEXT_DIRS:
        for path in sorted((ROOT / directory).rglob("*")):
            if not path.is_file() or path.suffix not in {".c", ".h", ".s", ".md", ".csv"}:
                continue
            rel = str(path.relative_to(ROOT))
            if rel in TABLES:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            if not pattern.search(text):
                continue
            touched.append(rel)
            if not args.dry_run:
                path.write_text(pattern.sub(new_name, text), encoding="utf-8")

    prefix = "[deneme] " if args.dry_run else ""
    print(f"{prefix}{address}: {old_name} -> {new_name}  ({len(touched)} files)")
    for rel in touched:
        print(f"  {rel}")


if __name__ == "__main__":
    main()
