#!/usr/bin/env python3
"""Compile the C sources under src/, compare them against the ROM, and produce
data/c_sources.csv.

This makes the distinction between "byte-matching from assembly" and
"byte-matching from C" measurable. An assembly transcription reproduces the ROM
but not readable source; the project's real goal is the latter.

Usage:  python3 tools/scan_c_sources.py
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import (  # noqa: E402
    DEFAULT_CC, ROM_BASE, compile_and_link, function_rows, rom_bytes,
)

ROOT = Path(__file__).resolve().parent.parent
SOURCES = ROOT / "src"
OUTPUT = ROOT / "data/c_sources.csv"


def main() -> None:
    rom = rom_bytes()
    rows = function_rows()
    records = []
    failures = []

    for source in sorted(SOURCES.rglob("*.c")):
        try:
            blob, layout, _ = compile_and_link(source, DEFAULT_CC)
        except SystemExit as error:
            # SKIPPING a source that fails to compile silently showed that
            # c_sources.csv'den sessizce silip komutu yine de basarili
            # file's functions as before: with a broken C file `make check`
            # stayed green. Now they are collected and reported as a failure
            # ve CSV'nin USTUNE YAZMIYORUZ.
            failures.append((source, error))
            continue
        relative = source.relative_to(ROOT)
        for name, (offset, size) in sorted(layout.items(), key=lambda kv: kv[1][0]):
            address = int(rows[name]["address"], 16)
            mapped_size = int(rows[name]["size"], 0)
            start = address - ROM_BASE
            # A short prefix produced by the compiler happening to agree is not
            # is not a function match. The symbol must cover at least the body
            # kapsamiyorsa matching terfisi yasaktir.
            complete = size >= mapped_size
            matched = complete and blob[offset:offset + size] == rom[start:start + size]
            records.append({
                "address": f"0x{address:08X}",
                "name": name,
                "source": str(relative),
                "compiled_size": size,
                "mapped_size": mapped_size,
                "matching": "yes" if matched else "no",
            })

    if failures:
        print(f"\nSTOPPED: {len(failures)} sources failed to compile; "
              f"{OUTPUT.relative_to(ROOT)} DEGISTIRILMEDI.", file=sys.stderr)
        for source, error in failures:
            print(f"  {source.relative_to(ROOT)}: {error}", file=sys.stderr)
        sys.exit(1)

    records.sort(key=lambda r: int(r["address"], 16))
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "address", "name", "source", "compiled_size", "mapped_size", "matching"
            ],
            lineterminator="\n",
        )
        writer.writeheader()
        writer.writerows(records)

    matched = sum(1 for r in records if r["matching"] == "yes")
    print(f"C sources: {len(records)} functions, {matched} byte-matching "
          f"-> {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
