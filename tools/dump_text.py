#!/usr/bin/env python3
"""Metin tablosunu YERELDE kendi ROM'undan okur.

Cikti build/ altina yazilir ve depoya girmez (docs/ROADMAP.md: ROM'dan
cikarilan varliklar paylasilmaz).

Kullanim:
    python3 tools/dump_text.py            # ozet
    python3 tools/dump_text.py --lang 4   # bir dilin dizelerini yaz
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


def read_cstr(rom, off, limit=1024):
    end = rom.find(b"\x00", off, off + limit)
    return rom[off:end if end >= 0 else off + limit]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lang", type=int, help=f"0..{LANG_COUNT - 1}")
    args = ap.parse_args()

    if not ROM.exists():
        print(f"ROM yok: {ROM}")
        return 1
    rom = ROM.read_bytes()
    ptrs = [struct.unpack_from("<I", rom, o)[0] - ROM_BASE
            for o in range(TABLE_START, TABLE_END, 4)]
    per = len(ptrs) // LANG_COUNT
    print(f"{len(ptrs)} giris, {LANG_COUNT} dil x {per} dize")

    if args.lang is None:
        for k in range(LANG_COUNT):
            seg = ptrs[k * per:(k + 1) * per]
            print(f"  dil {k}: {min(seg):#09x} .. {max(seg):#09x}")
        print("\nBir dili yazmak icin: --lang N  (cikti build/ altina)")
        return 0

    seg = ptrs[args.lang * per:(args.lang + 1) * per]
    OUT.mkdir(exist_ok=True)
    dest = OUT / f"text_lang{args.lang}.txt"
    with dest.open("w", encoding="utf-8") as fh:
        for i, p in enumerate(seg):
            s = read_cstr(rom, p).decode("ascii", "replace")
            fh.write(f"{i:04d}\t{p:#09x}\t{s}\n")
    print(f"yazildi: {dest.relative_to(ROOT)} ({len(seg)} dize)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
