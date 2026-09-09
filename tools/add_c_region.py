#!/usr/bin/env python3
"""Record a new C source as a verified ROM region.

First it verifies that the whole region matches from C; if it does not, nothing
is written. Then it adds an entry to data/matching_regions.csv and a build rule
to the Makefile, so that `make matching` audits the region from then on.

retire_asm.py converts an existing assembly region to C; this tool adds a ROM
area that was NEVER VERIFIED BEFORE to the coverage -- that is the real growth.

Usage:  python3 tools/add_c_region.py <module/name> <start_hex> <end_hex> "<note>"
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import compile_and_link, rom_bytes  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
REGIONS = ROOT / "data/matching_regions.csv"
MAKEFILE = ROOT / "Makefile"


def main() -> None:
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    key, start, end, note = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16), sys.argv[4]
    module, name = key.split("/")

    source = ROOT / f"src/{module}/{name}.c"
    blob, _, _ = compile_and_link(source)
    region = rom_bytes()[start:end]
    if blob != region:
        sys.exit(f"ERROR: the region does not match ({len(blob)} vs {len(region)} bytes). "
                 f"Nothing was written.")

    binary = f"build/{module}/{name}.bin"
    rows = list(csv.DictReader(REGIONS.open(newline="", encoding="utf-8")))
    if any(r["binary"] == binary for r in rows):
        sys.exit(f"ERROR: {binary} is already in matching_regions.csv.")
    rows.append({
        "start": f"0x{0x08000000 + start:08X}",
        "end": f"0x{0x08000000 + end:08X}",
        "binary": binary,
        "module": module,
        "notes": note,
    })
    fields = ["start", "end", "binary", "module", "notes"]
    with REGIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        for row in sorted(rows, key=lambda r: int(r["start"], 16)):
            writer.writerow({k: row.get(k, "") for k in fields})

    text = MAKEFILE.read_text()
    target = f"{module}-{name}-match".replace("_", "-")
    rule = (
        f"\nbuild/{module}/{name}.bin: src/{module}/{name}.c "
        f"data/functions.csv data/ram_map.csv\n"
        f"\t@mkdir -p build/{module}\n"
        f"\t@python3 tools/build_c.py $< $@\n\n"
        f"{target}: verify-rom build/{module}/{name}.bin\n"
        f"\t@python3 tools/compare_slice.py baserom.gba {start:#x} build/{module}/{name}.bin\n"
    )
    marker = "\nmatching: libc-verify"
    if marker not in text:
        sys.exit("ERROR: the 'matching:' target was not found in the Makefile.")
    text = text.replace(marker, rule + marker, 1)
    text = text.replace(marker, f"{marker[:-1]} {target}", 1) if False else text
    # add it to the matching target's dependency list
    lines = text.splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.startswith("matching: libc-verify"):
            lines[i] = line.rstrip("\n") + f" {target}\n"
            break
    MAKEFILE.write_text("".join(lines))
    print(f"{key}: added as a new verified region of {len(region)} bytes ({target}).")


if __name__ == "__main__":
    main()
