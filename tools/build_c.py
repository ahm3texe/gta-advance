#!/usr/bin/env python3
"""Compile a C source with agbcc, link it at its ROM address, and produce a .bin.

Usage:  python3 tools/build_c.py src/save/save_wrappers.c build/save/save_wrappers.bin
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import DEFAULT_CC, compile_and_link  # noqa: E402


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    compiler = next(
        (a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--cc=")),
        DEFAULT_CC,
    )
    if len(args) != 2:
        sys.exit(__doc__)
    source, output = Path(args[0]), Path(args[1])
    blob, _, _ = compile_and_link(source, compiler)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(blob)


if __name__ == "__main__":
    main()
