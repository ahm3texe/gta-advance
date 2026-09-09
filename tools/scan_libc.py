#!/usr/bin/env python3
"""Search the ROM for functions from agbcc's libc.a.

The ROM links against the newlib shipped with agbcc: the strings
"bug in vfprintf: bad base", "_sbrk: Heap and stack collision" and "Infinity"
appear verbatim both in the ROM and in tools/agbcc/lib/libc.a. So the tail of the
code region is standard library code that needs no reverse engineering; we
already have its source.

Functions containing external calls do not match the ROM bytes until they are
linked: the `bl` target and the addresses in the literal pool depend on the
binding. The scan is therefore MASKED - the bytes touched by relocation are
treated as wildcards and the rest of the body is searched verbatim. That way a
function can be found without knowing its address.

Usage:  python3 tools/scan_libc.py [--csv]
"""
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# agbcc iki kutuphane ile geliyor. Onceki surum yalnizca libc.a'ya bakiyordu;
# libgcc.a HIC taranmamisti, oysa ROM'un bolme/mod yardimcilari oradan geliyor.
LIBS = [
    ("libc", ROOT / "tools/agbcc/lib/libc.a"),
    ("libgcc", ROOT / "tools/agbcc/lib/libgcc.a"),
]
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000
MIN_SIZE = 16       # short bodies produce coincidental matches
MIN_FIXED = 12      # maskeleme sonrasi en az bu kadar sabit byte kalmali


def relocation_offsets(obj: Path) -> list[tuple[int, int]]:
    """(.text icindeki ofset, uzunluk) ciftleri."""
    out = subprocess.run(["arm-none-eabi-objdump", "-r", str(obj)],
                         capture_output=True, text=True).stdout
    spans, in_text = [], False
    for line in out.splitlines():
        if line.startswith("RELOCATION RECORDS FOR"):
            in_text = "[.text]" in line
            continue
        if not in_text or not line or line.startswith("OFFSET"):
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        try:
            offset = int(parts[0], 16)
        except ValueError:
            continue
        # Thumb BL 4 byte, ABS32 literal 4 byte; hepsini 4 byte kabul et.
        spans.append((offset, 4))
    return spans


def masked_find(rom: bytes, body: bytes, spans: list[tuple[int, int]],
                start: int) -> int:
    """Joker byte'lar iceren govdeyi ROM'da arar; bulunan mutlak ofseti doner."""
    mask = bytearray(b"\xff" * len(body))
    for offset, length in spans:
        for i in range(offset - start, offset - start + length):
            if 0 <= i < len(mask):
                mask[i] = 0
    fixed = [i for i, m in enumerate(mask) if m]
    if len(fixed) < MIN_FIXED:
        return -1
    # Use the longest fixed run as a coarse filter.
    runs, run = [], []
    for i in fixed:
        if run and i == run[-1] + 1:
            run.append(i)
        else:
            if run:
                runs.append(run)
            run = [i]
    if run:
        runs.append(run)
    anchor = max(runs, key=len)
    needle = body[anchor[0]:anchor[-1] + 1]
    if len(needle) < 4:
        return -1
    position = rom.find(needle)
    while position >= 0:
        base = position - anchor[0]
        if base >= 0 and base + len(body) <= len(rom):
            candidate = rom[base:base + len(body)]
            if all(candidate[i] == body[i] for i in fixed):
                return base
        position = rom.find(needle, position + 1)
    return -1


SCAN_STATS = {"checked": 0, "masked": 0}


def scan_archive(rom: bytes, work: Path, binary: Path, found: list,
                 lib_name: str) -> None:
    """Bir arsivdeki .o'lari tarayip bulunanlari `found` icine ekler."""
    for obj in sorted(work.glob("*.o")):
        nm = subprocess.run(["arm-none-eabi-nm", "-S", str(obj)],
                            capture_output=True, text=True).stdout
        symbols = [
            (p[3], int(p[0], 16), int(p[1], 16))
            for p in (line.split() for line in nm.splitlines())
            if len(p) == 4 and p[2] in "tT"
        ]
        if not symbols:
            continue
        spans = relocation_offsets(obj)
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(binary)],
                       capture_output=True)
        if not binary.exists():
            continue
        text = binary.read_bytes()
        for name, offset, size in symbols:
            if size < MIN_SIZE or offset + size > len(text):
                continue
            SCAN_STATS["checked"] += 1
            body = text[offset:offset + size]
            local = [(o, l) for o, l in spans if offset <= o < offset + size]
            if local:
                index = masked_find(rom, body, local, offset)
                SCAN_STATS["masked"] += 1
            else:
                index = rom.find(body)
            if index >= 0:
                found.append((ROM_BASE + index, name, size, bool(local), lib_name))


def main() -> None:
    missing = [str(path) for _, path in LIBS if not path.exists()]
    if missing:
        sys.exit(f"{', '.join(missing)} missing. First run: make agbcc")
    if not ROM.exists():
        sys.exit("baserom.gba is missing.")

    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        known = {int(r["address"], 16) for r in csv.DictReader(handle)}

    found, checked, masked = [], 0, 0
    per_lib = {}
    for lib_name, lib_path in LIBS:
        work = Path(tempfile.mkdtemp())
        subprocess.run(["arm-none-eabi-ar", "x", str(lib_path)], cwd=work,
                       capture_output=True)
        binary = work / "slice.bin"
        before = len(found)
        scan_archive(rom, work, binary, found, lib_name)
        per_lib[lib_name] = len(found) - before
    checked = SCAN_STATS["checked"]
    masked = SCAN_STATS["masked"]

    if False:
      for obj in sorted(work.glob("*.o")):
        nm = subprocess.run(["arm-none-eabi-nm", "-S", str(obj)],
                            capture_output=True, text=True).stdout
        symbols = [
            (p[3], int(p[0], 16), int(p[1], 16))
            for p in (line.split() for line in nm.splitlines())
            if len(p) == 4 and p[2] in "tT"
        ]
        if not symbols:
            continue
        spans = relocation_offsets(obj)
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(binary)],
                       capture_output=True)
        text = binary.read_bytes()
        for name, offset, size in symbols:
            if size < MIN_SIZE or offset + size > len(text):
                continue
            checked += 1
            body = text[offset:offset + size]
            local = [(o, l) for o, l in spans if offset <= o < offset + size]
            if local:
                index = masked_find(rom, body, local, offset)
                masked += 1
            else:
                index = rom.find(body)
            if index >= 0:
                found.append((ROM_BASE + index, name, size, bool(local)))

    # Symbols with identical bodies (e.g. toupper/_toupper) find the same address.
    # The bytes cannot tell which one sits at that address; rather than invent
    # they are marked as ambiguous.
    grouped: dict[int, list] = {}
    for address, name, size, was_masked, lib_name in found:
        grouped.setdefault(address, []).append((name, size, was_masked, lib_name))
    found = []
    for address in sorted(grouped):
        entries = grouped[address]
        names = sorted(n for n, _, _, _ in entries)
        _, size, was_masked, lib_name = entries[0]
        found.append((address, names[0], size, was_masked, names[1:], lib_name))

    if "--csv" in sys.argv:
        print("address,name,size,status,module,notes")
        for address, name, size, was_masked, aliases, lib_name in found:
            note = (f"{lib_name} routine; matched against agbcc {lib_name}.a build"
                    if was_masked else
                    f"{lib_name} routine; body byte-identical to agbcc {lib_name}.a build")
            if aliases:
                note += (f"; identical body to {', '.join(aliases)} - which symbol "
                         f"occupies this address is undetermined")
            if address not in known:
                note += "; missed by Ghidra auto-analysis"
            status = "discovered" if aliases else "documented"
            print(f'0x{address:08X},{name},{size},{status},{lib_name},"{note}"')
        return

    detail = ", ".join(f"{k}: {v}" for k, v in per_lib.items())
    print(f"{checked} functions searched ({masked} of them masked)")
    print(f"ROM'da bulunan: {len(found)}  ({detail})\n")
    for address, name, size, was_masked, aliases, lib_name in found:
        flags = []
        if aliases:
            flags.append(f"belirsiz: {'/'.join([name] + aliases)}")
        if was_masked:
            flags.append("maskeli")
        if address not in known:
            flags.append("Ghidra kacirmis")
        flags.append(lib_name)
        tail = f"   <- {', '.join(flags)}"
        print(f"  0x{address:08X}  {name:20} {size:>4}B{tail}")


if __name__ == "__main__":
    main()
