#!/usr/bin/env python3
"""Switch a region to the C build and retire its assembly source.

First it verifies that the whole region matches from C; if it does not, nothing
is deleted. Then it changes the Makefile rule and removes the .s/.ld files.

Usage:    python3 tools/retire_asm.py <module/name> <start_hex> <end_hex>
Example:  python3 tools/retire_asm.py ui/menu_graphics 0x1E30 0x1F04
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import compile_and_link, rom_bytes  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    key, start, end = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)
    module, name = key.split("/")

    source = ROOT / f"src/{module}/{name}.c"
    blob, _, _ = compile_and_link(source)
    region = rom_bytes()[start:end]
    if blob != region:
        sys.exit(f"ERROR: the region does not match ({len(blob)} vs {len(region)} bytes). "
                 f"Nothing was deleted.")

    makefile = ROOT / "Makefile"
    text = makefile.read_text()
    pattern = re.compile(
        rf"build/{module}/{name}\.o: src/{module}/{name}\.s\n.*?"
        rf"build/{module}/{name}\.bin: build/{module}/{name}\.elf\n[^\n]*\n",
        re.S,
    )
    replacement = (
        f"# Built from the C source: the assembly equivalent was retired.\n"
        f"build/{module}/{name}.bin: src/{module}/{name}.c "
        f"data/functions.csv data/ram_map.csv\n"
        f"\t@mkdir -p build/{module}\n"
        f"\t@python3 tools/build_c.py $< $@\n"
    )
    if not pattern.search(text):
        sys.exit(f"ERROR: the build/{module}/{name} rule was not found in the Makefile.")
    makefile.write_text(pattern.sub(replacement, text, count=1))

    for path in (ROOT / f"src/{module}/{name}.s", ROOT / f"config/{name}.ld"):
        if path.exists():
            path.unlink()
    print(f"{key}: {len(region)} bytes match from C, the assembly was retired.")


if __name__ == "__main__":
    main()
