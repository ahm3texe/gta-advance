#!/usr/bin/env python3
"""Write objdiff.json, the manifest objdiff reads to pair target with base.

objdiff needs one *unit* per translation unit, each naming two object files:
the target (what the cartridge contains, synthesised by tools/gen_expected.py)
and the base (what our source compiles to, written by tools/agbcc_build.py as
`build/cmatch/<stem>.o`).  data/c_sources.csv is already the single source of
truth for which C files are build targets, so the manifest is derived from it
rather than maintained by hand -- a hand-written list would drift the moment a
source is added, and a unit pointing at a missing object is a dead row in the
UI.

A unit is emitted ONLY when its target object exists.  gen_expected.py writes
an object only after proving byte for byte that it reproduces the ROM, so
"the file is there" is exactly "the target was verified".  Listing a unit whose
target is missing or unverified would put a confident diff against fiction in
front of the reader, which is worse than the unit not being listed at all.

The schema is https://github.com/encounter/objdiff/blob/main/config.schema.json;
it marks no field as required, so only fields that exist there are written.

Usage:
    python3 tools/gen_objdiff.py            # writes objdiff.json at the root
    python3 tools/gen_objdiff.py --check    # fail if the file is out of date
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
C_SOURCES = ROOT / "data/c_sources.csv"
EXPECTED_DIR = ROOT / "expected"
BASE_OBJ_DIR = ROOT / "build/cmatch"
OUTPUT = ROOT / "objdiff.json"

SCHEMA = "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json"
MIN_VERSION = "3.0.0"


def unit_name(source: str) -> str:
    """`src/world/actor_init.c` -> `world/actor_init`, which is what the UI shows."""
    return str(Path(source).with_suffix("")).removeprefix("src/")


def build_config(expected_dir: Path) -> dict:
    """Assemble the manifest from the CSV, keeping only verified targets."""
    with C_SOURCES.open(newline="", encoding="utf-8") as handle:
        sources = sorted({row["source"] for row in csv.DictReader(handle) if row["source"]})

    units: list[dict] = []
    for source in sources:
        target = expected_dir / Path(source).with_suffix(".o")
        base = BASE_OBJ_DIR / (Path(source).stem + ".o")
        if not target.exists():
            continue
        units.append({
            "name": unit_name(source),
            "target_path": target.relative_to(ROOT).as_posix(),
            "base_path": base.relative_to(ROOT).as_posix(),
            "metadata": {"source_path": source},
        })

    return {
        "$schema": SCHEMA,
        "min_version": MIN_VERSION,
        "custom_make": "make",
        # Left off deliberately. objdiff would run `make <base_path>`, which
        # falls through to make's implicit .s -> .o rule and reassembles the
        # stale assembly left in build/cmatch by the previous build. Editing a
        # C file and pressing rebuild would then show the OLD code as if it
        # were new. Rebuild with `make c-match FILE=src/...` instead.
        "build_base": False,
        "watch_patterns": ["src/**/*.c", "include/**/*.h"],
        "units": units,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-o", "--output", type=Path, default=OUTPUT,
                        help="where the manifest goes (default: objdiff.json)")
    parser.add_argument("--expected", type=Path, default=EXPECTED_DIR,
                        help="directory holding the verified target objects")
    parser.add_argument("--check", action="store_true",
                        help="do not write; exit non-zero if the file is stale")
    args = parser.parse_args()

    if not C_SOURCES.exists():
        print(f"{C_SOURCES} is missing", file=sys.stderr)
        return 2
    if not args.expected.is_dir():
        print(f"{args.expected} does not exist. First run: make expected", file=sys.stderr)
        return 2

    config = build_config(args.expected)
    if not config["units"]:
        print(f"no verified target object under {args.expected}; "
              f"run tools/gen_expected.py first", file=sys.stderr)
        return 1

    text = json.dumps(config, indent=2) + "\n"
    if args.check:
        current = args.output.read_text(encoding="utf-8") if args.output.exists() else ""
        if current != text:
            print(f"{args.output} is out of date; run: make objdiff", file=sys.stderr)
            return 1
        print(f"{args.output} is up to date ({len(config['units'])} units)")
        return 0

    args.output.write_text(text, encoding="utf-8")
    print(f"{args.output}: {len(config['units'])} units "
          f"({len(config['units'])} verified targets)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
