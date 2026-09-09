#!/usr/bin/env python3
"""Reproduce the SIO TX pointer-advance experiment against the ROM.

The source is not modified; the three candidates and the build artifacts are kept
in a temporary directory. The score uses the same disassembly/alignment
computation as tools/diff_function.py. This experiment is neither a full
byte-match nor a semantic test of the whole driver.
"""
import difflib
import re
import tempfile
from collections import Counter
from pathlib import Path

import agbcc_build as build
import diff_function as diff


SOURCE = build.ROOT / "src/world/sio_driver.c"
TARGET = "FUN_080657d8"
HELPER = "PackLocalLinkTag"


def replace_once(source: str, old: str, new: str) -> str:
    if source.count(old) != 1:
        raise ValueError(f"Deney capasi degisti: {old!r}")
    return source.replace(old, new, 1)


def variants(source: str) -> dict[str, str]:
    collapsed = replace_once(
        source,
        "    entry = gRam020003C0;\n    entry += index;",
        "    entry = gRam020003C0 + index;",
    )
    direct, removed = re.subn(
        rf"static inline u16 {HELPER}\(u32 index\)\n\{{.*?\n\}}\n",
        "", source, count=1, flags=re.S,
    )
    if removed != 1:
        raise ValueError("Etiket yardimcisinin tanimi bulunamadi")
    for field, index in [("tag", "gRam0200048C"), ("endTag", "k")]:
        direct = replace_once(
            direct,
            f"FRAME(tx2)->tx.{field} = {HELPER}({index});",
            f"FRAME(tx2)->tx.{field} =\n"
            f"                      (gRam02000E80[{index} & RING_MASK] << 8)\n"
            f"                    |  gRam020003C0[{index} & RING_MASK];",
        )
    return {"previous direct expression": direct,
            "tek adimli isaretci": collapsed,
            "korunan iki adim": source}


def main() -> None:
    row = build.function_rows()[TARGET]
    address, rom_size = int(row["address"], 16), int(row["size"])
    rom = build.rom_bytes()
    rom_start = address - build.ROM_BASE
    calls = []

    with tempfile.TemporaryDirectory(prefix="sio-tx-") as work:
        # Write to no persistent artifact, including the shared build/cmatch files.
        build.BUILD = diff.BUILD = Path(work) / "build"
        print(f"{TARGET} @ 0x{address:08X}; ROM {rom_size} bytes")
        print(f"{'candidate':26} {'size':>6} {'same/total insns':>19}")
        for i, (label, source) in enumerate(variants(SOURCE.read_text()).items()):
            path = Path(work) / f"variant_{i}.c"
            path.write_text(source)
            blob, layout, _ = build.compile_and_link(path)
            offset, size = layout[TARGET]
            mine = blob[offset:offset + size]
            theirs = rom[rom_start:rom_start + max(rom_size, size)]
            rom_asm = diff.disassemble(theirs, address, tag=f"rom_{i}")
            our_asm = diff.disassemble(mine, address, tag=f"mine_{i}")
            same = sum(m.size for m in difflib.SequenceMatcher(
                None, rom_asm, our_asm, autojunk=False,
            ).get_matching_blocks())
            total = max(len(rom_asm), len(our_asm))
            print(f"{label:26} {size:6} {same:>10}/{total:<8}")
            assembly = (build.BUILD / f"{path.stem}.s").read_text()
            calls.append(Counter(re.findall(r"^\s*bl\s+(\w+)", assembly, re.M)))

    if not all(c == calls[0] for c in calls[1:]) or HELPER in calls[-1]:
        raise SystemExit("ERROR: external call targets/counts changed")
    print("External call targets and counts are identical in all three candidates; the helper was inlined.")
    print("Komut skoru kismi olcumdur; tam kabul olcutu make c-match'tir.")


if __name__ == "__main__":
    main()
