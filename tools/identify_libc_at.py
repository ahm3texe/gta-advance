#!/usr/bin/env python3
"""Identify which libc.a function sits at a given ROM address.

scan_libc.py searches blindly: it looks for the body across the WHOLE ROM.
Because the bytes touched by relocation are treated as wildcards, short
functions with heavy masking (e.g. newlib's `_xxx_r` reentrant wrappers) are
either not found or cannot be told apart in that search.

This tool does the reverse: the address is KNOWN, and the question is which
function it is. Because it compares at a single point, even heavily masked
bodies become distinguishable.

Usage:
    python3 tools/identify_libc_at.py 0x08071D20 [0x08071D5C ...]
"""
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from scan_libc import relocation_offsets  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
LIBC = ROOT / "tools/agbcc/lib/libc.a"
ROM_BASE = 0x08000000
# Weak matches produce false positives: the 8 fixed bytes of a 12-byte body fit
# EVERY function of the same shape. In this repository 0x08071D50 was mistaken
# for `__errno` that way; it was eliminated once the pool value turned out to be
# a ROM address. Hence both an absolute and a proportional lower bound.
MIN_FIXED = 12
MIN_FIXED_RATIO = 0.6


def text_symbols(obj: Path) -> list[tuple[str, int, int]]:
    """(name, offset within .text, size) -- only functions in .text."""
    out = subprocess.run(["arm-none-eabi-nm", "-S", "--defined-only", str(obj)],
                         capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in ("T", "t"):
            syms.append((parts[3], int(parts[0], 16), int(parts[1], 16)))
    return syms


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    targets = [int(a, 16) for a in sys.argv[1:]]
    rom = ROM.read_bytes()

    work = Path(tempfile.mkdtemp())
    members = subprocess.run(["arm-none-eabi-ar", "t", str(LIBC)],
                             capture_output=True, text=True).stdout.split()
    subprocess.run(["arm-none-eabi-ar", "x", str(LIBC)], cwd=work, check=True)

    candidates = []
    for name in members:
        obj = work / name
        if not obj.exists():
            continue
        text = work / f"{name}.text"
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(text)],
                       capture_output=True)
        if not text.exists() or text.stat().st_size == 0:
            continue
        blob = text.read_bytes()
        spans = relocation_offsets(obj)
        for sym, offset, size in text_symbols(obj):
            if size == 0 or offset + size > len(blob):
                continue
            body = blob[offset:offset + size]
            mask = bytearray(b"\xff" * size)
            for rel, length in spans:
                for i in range(rel - offset, rel - offset + length):
                    if 0 <= i < size:
                        mask[i] = 0
            fixed = [i for i, m in enumerate(mask) if m]
            candidates.append((name, sym, body, fixed))

    print(f"libc.a: {len(candidates)} function bodies prepared\n")
    for address in targets:
        off = address - ROM_BASE
        hits = []
        for name, sym, body, fixed in candidates:
            if len(fixed) < MIN_FIXED or len(fixed) < MIN_FIXED_RATIO * len(body):
                continue
            if off + len(body) > len(rom):
                continue
            chunk = rom[off:off + len(body)]
            if all(chunk[i] == body[i] for i in fixed):
                hits.append((sym, name, len(body), len(fixed)))
        print(f"0x{address:08X}:")
        if not hits:
            print("  no match")
        for sym, name, size, nfixed in sorted(hits, key=lambda h: -h[3]):
            print(f"  {sym:24s} {name:20s} {size:4d}B  "
                  f"{nfixed}/{size} fixed bytes "
                  f"({100 * nfixed // size}%)")


if __name__ == "__main__":
    main()
