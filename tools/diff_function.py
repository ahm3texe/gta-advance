#!/usr/bin/env python3
"""Show a function's ROM form beside the form compiled from your C.

Usage:
    python3 tools/diff_function.py src/save/save_helpers.c WriteU16LE [--cc=agbcc]

The left column is the real code in the ROM, the right column the code
generated from your C. Differing and missing instructions are marked. This
answers "where does it diverge" when fixing a non-matching function.
"""
import difflib
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import (  # noqa: E402
    BUILD, DEFAULT_CC, ROM_BASE, compile_and_link, function_rows, rom_bytes, run,
)

GREEN, RED, DIM, RESET = "\033[32m", "\033[31m", "\033[2m", "\033[0m"


def disassemble(blob: bytes, base: int, thumb: bool = True,
                tag: str = "dis") -> list[str]:
    # The file name is call-specific so that parallel tools do not clobber each other.
    raw = BUILD / f"{tag}.bin"
    raw.write_bytes(blob)
    out = run([
        "arm-none-eabi-objdump", "-b", "binary", "-m", "arm7tdmi",
        "-M", "force-thumb" if thumb else "no-force-thumb",
        "-D", f"--adjust-vma={base:#x}", str(raw),
    ])
    return [
        re.sub(r"\s+", " ", m.group(1)).strip()
        for m in (re.match(r"\s*[0-9a-f]+:\s+[0-9a-f ]+\t(.*)", l) for l in out.splitlines())
        if m
    ]


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    compiler = next(
        (a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--cc=")),
        DEFAULT_CC,
    )
    if len(args) != 2:
        sys.exit(__doc__)
    source, target = Path(args[0]), args[1]

    rows = function_rows()
    if target not in rows:
        sys.exit(f"{target} is not in data/functions.csv")

    blob, layout, _ = compile_and_link(source, compiler)
    if target not in layout:
        sys.exit(f"{target} is not defined in {source}")

    address = int(rows[target]["address"], 16)
    rom_size = int(rows[target]["size"] or 0)
    thumb = "ARM" not in rows[target]["notes"].upper().split()
    offset, size = layout[target]

    mine = blob[offset:offset + size]
    start = address - ROM_BASE
    # The size in data/functions.csv is Ghidra's body estimate and may leave out
    # the literal pool and alignment padding. Truncating the ROM side with it
    # produces spurious "NOT IN ROM" lines even for a fully matching function.
    # Take the larger of the two sides.
    theirs = rom_bytes()[start:start + max(rom_size, size)]

    rom_asm = disassemble(theirs, address, thumb, f"{source.stem}.rom")
    our_asm = disassemble(mine, address, thumb, f"{source.stem}.mine")

    print(f"{target}  @ 0x{address:08X}  "
          f"ROM {len(theirs)} bytes / yours {len(mine)} bytes  [{compiler}]")
    print(f"{'ROM (target)':38}   your C output")
    print("-" * 78)

    same = 0
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(
        None, rom_asm, our_asm, autojunk=False
    ).get_opcodes():
        if tag == "equal":
            for line in rom_asm[i1:i2]:
                same += 1
                print(f"{DIM}{line:38} = {line}{RESET}")
        elif tag == "replace":
            for k in range(max(i2 - i1, j2 - j1)):
                left = rom_asm[i1 + k] if i1 + k < i2 else ""
                right = our_asm[j1 + k] if j1 + k < j2 else ""
                print(f"{RED}{left:38} ≠ {right}{RESET}")
        elif tag == "delete":
            for line in rom_asm[i1:i2]:
                print(f"{RED}{line:38} <- MISSING in yours{RESET}")
        elif tag == "insert":
            for line in our_asm[j1:j2]:
                print(f"{RED}{'NOT IN ROM':38} -> {line}{RESET}")

    print("-" * 78)
    total = max(len(rom_asm), len(our_asm))
    print(f"{GREEN}BYTE-MATCHING{RESET}" if mine == theirs
          else f"{same}/{total} instructions identical, {total - same} differ")
    sys.exit(0 if mine == theirs else 1)


if __name__ == "__main__":
    main()
