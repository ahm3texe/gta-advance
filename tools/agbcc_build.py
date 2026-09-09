#!/usr/bin/env python3
"""Shared layer that compiles a C source with agbcc and links it at its ROM address.

Linking is required: `bl` instructions do not produce the correct bytes until
the target address is resolved, so no non-leaf function can be verified without
it. External symbols are resolved from the ROM addresses in data/functions.csv.
"""
import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AGBCC_DIR = ROOT / "tools/agbcc/bin"
ROM_PATH = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
RAM_MAP = ROOT / "data/ram_map.csv"
BUILD = ROOT / "build/cmatch"

DEFAULT_CC = "old_agbcc"
CC1FLAGS = ["-mthumb-interwork", "-O2", "-fhex-asm"]

# ARM mode. The 18 functions in the ROM's 0x0806xxxx region are in ARM mode and
# until now COULD NOT BE COMPILED: the chain was tied to Thumb only.
# `agbcc_arm` had been sitting in the toolchain from the start.
# This path is used when the source file contains an ARM_MARKER line.
ARM_MARKER = "MODE: ARM"
ARM_CC = "agbcc_arm"
# `-fhex-asm` is NOT RECOGNIZED by agbcc_arm (measured); the others are accepted.
# -fomit-frame-pointer MEASURED: without it agbcc_arm builds an APCS frame
# (mov ip,sp / stmfd {fp,ip,lr,pc} / sub fp,ip,#4) and the output grows by
# about 48 bytes; the ROM uses a plain push.
# -fno-schedule-insns / -fno-schedule-insns2 MEASURED: without them agbcc_arm
# built a stack frame and spilled intermediates. On the first ARM candidate the
# effect was 240 -> 216 -> 188 bytes (ROM 196). Added permanently.
ARM_CC1FLAGS = [
    "-mthumb-interwork", "-O2", "-fomit-frame-pointer",
    "-fno-schedule-insns", "-fno-schedule-insns2",
]
ROM_BASE = 0x08000000


def run(cmd: list[str], stdout: Path | None = None) -> str:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"ERROR: {' '.join(cmd)}\n{result.stderr.strip()}")
    if stdout is not None:
        stdout.write_text(result.stdout, encoding="utf-8")
    return result.stdout


def function_rows() -> dict[str, dict[str, str]]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        return {row["name"]: row for row in csv.DictReader(handle)}


def ram_rows() -> dict[str, dict[str, str]]:
    """RAM symbols: external symbols without a code address resolve from here."""
    if not RAM_MAP.exists():
        return {}
    with RAM_MAP.open(newline="", encoding="utf-8") as handle:
        return {row["name"]: row for row in csv.DictReader(handle)}


def _symbols(obj: Path) -> dict[str, tuple[int, int]]:
    out = run(["arm-none-eabi-nm", "-S", str(obj)])
    return {
        parts[3]: (int(parts[0], 16), int(parts[1], 16))
        for parts in (line.split() for line in out.splitlines())
        if len(parts) == 4 and parts[2] in "tT"
    }


def _undefined(obj: Path) -> list[str]:
    out = run(["arm-none-eabi-nm", "-u", str(obj)])
    return [
        parts[-1]
        for parts in (line.split() for line in out.splitlines())
        if parts and parts[0] == "U"
    ]


def compile_and_link(source: Path, compiler: str = DEFAULT_CC):
    """Compile a C file, link it at its ROM address and return the results.

    Returns: (linked binary, {function: (offset, size)}, base address)
    Ofsetler ikilinin basina goredir.
    """
    agbcc = AGBCC_DIR / compiler
    if not agbcc.exists():
        sys.exit(f"{compiler} is not installed. First run: make agbcc")
    if not ROM_PATH.exists():
        sys.exit("baserom.gba is missing. First run: make prepare-rom ROM_ZIP=...")

    BUILD.mkdir(parents=True, exist_ok=True)
    stem = BUILD / source.stem

    is_arm = ARM_MARKER in source.read_text(encoding="utf-8")
    if is_arm:
        agbcc = AGBCC_DIR / ARM_CC
        if not agbcc.exists():
            sys.exit(f"{ARM_CC} is not installed. First run: make agbcc")
        flags = ARM_CC1FLAGS
    else:
        flags = CC1FLAGS

    run(["cpp", "-nostdinc", "-undef", f"-I{ROOT / 'include'}", str(source)],
        Path(f"{stem}.i"))
    run([str(agbcc), *flags, "-o", f"{stem}.s", f"{stem}.i"])
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.probe.o", f"{stem}.s"])

    rows = function_rows()
    # External symbols are given to the assembler with .equ, so that `bl` is
    # encoded here and never reaches the linker. Left to the linker, the
    # absolute symbol is not recognised as a Thumb function, an interworking
    # veneer is inserted, and the `bl` target comes out wrong.
    ram = ram_rows()
    externs = []
    for name in _undefined(Path(f"{stem}.probe.o")):
        # The `__thumb` suffix resolves to the symbol address | 1. Stored
        # function pointers must have the Thumb bit set; in a `bl` target
        # adding the bit would break the branch offset, so a SEPARATE name is used.
        thumb = name.endswith("__thumb")
        key = name[: -len("__thumb")] if thumb else name
        row = rows.get(key) or ram.get(key)
        if row is None:
            sys.exit(f"'{name}' is in neither data/functions.csv nor "
                     f"data/ram_map.csv; its address cannot be resolved")
        value = int(row["address"], 16) | (1 if thumb else 0)
        externs.append(f"    .equ {name}, {value:#x}\n")
    source_text = Path(f"{stem}.s").read_text(encoding="utf-8")
    # Section-end padding: `as` pads Thumb sections with NOP (0x46C0) by
    # default, while the ROM pads with zero. Explicit alignment fixes this.
    # ARM instructions are 4 bytes, Thumb 2. Wrong alignment leaves extra
    # padding at the section end and shifts the size.
    align = "4" if is_arm else "2"
    Path(f"{stem}.s").write_text(
        "".join(externs) + source_text + f"\n    .align {align}, 0\n",
        encoding="utf-8",
    )
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.o", f"{stem}.s"])

    obj = Path(f"{stem}.o")
    defined = _symbols(obj)

    known = [rows[name] for name in defined if name in rows]
    if not known:
        sys.exit(f"no function in {source} is in data/functions.csv")
    base = min(int(row["address"], 16) for row in known)

    # The section address is set explicitly: agbcc aligns .text to 8, and if the
    # base is not a multiple of 8 the linker pushes the section forward and
    # shifts every measurement.
    script = Path(f"{stem}.ld")
    script.write_text(
        "SECTIONS\n{\n"
        f"    . = {base:#x};\n"
        f"    .text {base:#x} : SUBALIGN(1) {{ *(.text) }}\n"
        + "    /DISCARD/ : { *(.comment) *(.note*) }\n}\n",
        encoding="utf-8",
    )
    run(["arm-none-eabi-ld", "-T", str(script), "-o", f"{stem}.elf", f"{stem}.o"])
    run(["arm-none-eabi-objcopy", "-O", "binary", "--only-section=.text",
         f"{stem}.elf", f"{stem}.bin"])

    linked = _symbols(Path(f"{stem}.elf"))
    layout = {
        name: (address - base, size)
        for name, (address, size) in linked.items()
        if name in rows
    }
    return Path(f"{stem}.bin").read_bytes(), layout, base


def rom_bytes() -> bytes:
    return ROM_PATH.read_bytes()
