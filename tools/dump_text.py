#!/usr/bin/env python3
"""Read the text table LOCALLY from your own ROM.

The output is written under build/ and does not enter the repository
(docs/ROADMAP.md: assets extracted from the ROM are not shared).

Usage:
    python3 tools/dump_text.py            # summary
    python3 tools/dump_text.py --lang 4   # write one language's strings
"""
import argparse
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
OUT = ROOT / "build"

TABLE_START = 0x0EC46D4
TABLE_END = 0x0EC771C
ROM_BASE = 0x08000000
LANG_COUNT = 5
LANG_NAMES = ["English", "Spanish", "French", "Italian", "German"]


def read_cstr(rom, off, limit=1024):
    end = rom.find(b"\x00", off, off + limit)
    return rom[off:end if end >= 0 else off + limit]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lang", type=int, help=f"0..{LANG_COUNT - 1}")
    args = ap.parse_args()

    if not ROM.exists():
        print(f"no ROM: {ROM}")
        return 1
    rom = ROM.read_bytes()
    ptrs = [struct.unpack_from("<I", rom, o)[0] - ROM_BASE
            for o in range(TABLE_START, TABLE_END, 4)]
    per = len(ptrs) // LANG_COUNT
    print(f"{len(ptrs)} entries, {LANG_COUNT} languages x {per} strings")

    if args.lang is None:
        for k in range(LANG_COUNT):
            seg = ptrs[k * per:(k + 1) * per]
            print(f"  language {k} ({LANG_NAMES[k]}): {min(seg):#09x} .. {max(seg):#09x}")
        print("\nTo write one language: --lang N  (output under build/)")
        return 0

    seg = ptrs[args.lang * per:(args.lang + 1) * per]
    OUT.mkdir(exist_ok=True)
    dest = OUT / f"text_lang{args.lang}.txt"
    with dest.open("w", encoding="utf-8") as fh:
        for i, p in enumerate(seg):
            # The encoding is LATIN-1 (ISO-8859-1), NOT ASCII: 0xC9=E-acute,
            # 0xD1=N-tilde, 0xC1=A-acute. Assuming ASCII made the accented
            # languages unreadable.
            s = read_cstr(rom, p).decode("latin-1")
            fh.write(f"{i:04d}\t{p:#09x}\t{s}\n")
    print(f"written: {dest.relative_to(ROOT)} ({len(seg)} strings)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
