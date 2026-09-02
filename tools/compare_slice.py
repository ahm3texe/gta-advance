#!/usr/bin/env python3
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: compare_slice.py ROM OFFSET BINARY", file=sys.stderr)
        return 2

    rom_path = Path(sys.argv[1])
    offset = int(sys.argv[2], 0)
    binary_path = Path(sys.argv[3])
    expected = binary_path.read_bytes()
    actual = rom_path.read_bytes()[offset:offset + len(expected)]

    if actual == expected:
        print(f"MATCH: {binary_path} == {rom_path}[0x{offset:X}:0x{offset + len(expected):X}] ({len(expected)} bytes)")
        return 0

    first = next((index for index, pair in enumerate(zip(actual, expected)) if pair[0] != pair[1]), None)
    if first is None:
        first = min(len(actual), len(expected))
    print(f"MISMATCH at ROM+0x{offset + first:X}: expected/generated lengths {len(actual)}/{len(expected)}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

