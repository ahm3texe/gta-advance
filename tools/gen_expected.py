#!/usr/bin/env python3
"""Synthesise objdiff's *target* objects (`expected/**/*.o`) out of the ROM.

objdiff diffs two ELF objects per translation unit: the *base*, which is what
our source compiles to today (`build/cmatch/<stem>.o`, produced by
tools/agbcc_build.py), and the *target*, which is what the cartridge actually
contains.  We have no original object files -- only baserom.gba -- so the
target side has to be manufactured.  Nothing in the repository did that, which
is why objdiff could not be used at all.

WHY THIS IS NOT A HEXDUMP
-------------------------
The naive target ("cut the function out of the ROM and emit `.2byte` for every
halfword") is worse than useless.  objdiff does not guess where code stops and
a literal pool starts; it reads the ARM mapping symbols `$t` (Thumb code),
`$a` (ARM code) and `$d` (data) out of the ELF.  GNU as emits `$d` at *every*
data directive inside a code section, so a hexdump target is marked data from
end to end and every single row reads as a mismatch against a base whose code
is properly marked `$t`.  The sibling project Dream-Atelier/kl-eod-decomp hit
exactly this in scripts/generate_expected.py.

So the code has to be emitted as real instructions and only the pools as data
directives, and the code/data split has to come from somewhere trustworthy:

  (a) If a function's status in data/functions.csv is `matching`, our own build
      already reproduces its ROM bytes exactly.  Then the `$t`/`$d` markers in
      `build/cmatch/<stem>.o` are not a guess about the ROM, they are a
      measurement of it, so they are read straight out with `readelf -sW` and
      re-applied at the function's ROM address.  This covers most of the corpus.
  (b) Otherwise the split is derived from the ROM: every word a `ldr rN,[pc,#..]`
      points at is data, iterated to a fixed point (a pool word decoded as an
      instruction can invent a fake pc-relative load, so one pass is not
      enough), plus the halfword of alignment padding that precedes a pool and
      any all-zero tail.

WHY NO RELOCATIONS
------------------
`bl` and `b` are written as PC-relative expressions (`bl . + 0x1234`) instead
of as references to an absolute symbol.  An absolute symbol would make `as`
leave a relocation behind, and a relocation means the bytes in the object are
*not* the bytes in the ROM -- kl-eod-decomp has to mask its relocated fields
before it can compare at all.  `. + N` is folded at assembly time, is byte-
identical to the cartridge because the branch is PC-relative anyway, and keeps
the verification below exact rather than approximate.

THE VERIFICATION CONTRACT
-------------------------
Every object is assembled, flattened with `objcopy -O binary` and compared byte
for byte against the ROM slice it claims to represent.  An object that fails is
discarded, never written, and reported as skipped.  A target that does not
reproduce the cartridge is strictly worse than a missing target, because
objdiff would then render a confident diff against fiction.

Usage:
    python3 tools/gen_expected.py                 # all of data/c_sources.csv
    python3 tools/gen_expected.py --only world/   # substring filter on source
"""
from __future__ import annotations

import argparse
import concurrent.futures
import csv
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT / "baserom.gba"
C_SOURCES = ROOT / "data/c_sources.csv"
FUNCTIONS = ROOT / "data/functions.csv"
BASE_OBJ_DIR = ROOT / "build/cmatch"
EXPECTED_DIR = ROOT / "expected"
ROM_BASE = 0x08000000

AS = "arm-none-eabi-as"
OBJCOPY = "arm-none-eabi-objcopy"
OBJDUMP = "arm-none-eabi-objdump"
READELF = "arm-none-eabi-readelf"

# The base object that agbcc_build.py produces puts everything in `.text`.
# objdiff pairs sections before it pairs symbols, so the target must use the
# same name or the two objects have nothing in common to compare.
SECTION_NAME = ".text"

# Functions of one source that sit this close together in the ROM are treated
# as one run and the bytes in between (inter-function alignment padding) are
# emitted verbatim, so the run stays byte-comparable against the ROM.  Anything
# further apart is a different part of the cartridge -- src/misc/empty_stubs.c
# collects stubs that are 60 KB apart -- and is emitted as a separate run,
# concatenated without the intervening ROM.
RUN_GAP_LIMIT = 64

# `as` is asked not to warn: a Thumb section whose alignment is 2 warns on
# every `ldr rN,[pc,#imm]`, and raising the alignment to 4 would make `as` pad
# the section tail and break the byte comparison below.  The alignment does not
# affect the encoding -- the immediate is taken literally -- which the byte
# comparison proves for every object that is written.
AS_FLAGS = ["-mcpu=arm7tdmi", "-mthumb-interwork", "-W"]

ARM_MARKER = "MODE: ARM"

# objdump line: "  8000430:\tb5f0      \tpush\t{r4, r5, r6, r7, lr}"
DISASM_LINE = re.compile(r"^\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*(?:\t(.*))?$")
# A pc-relative literal load, which is what marks a word as pool data.
LDR_PC = re.compile(r"\bldr(?:\.[nw])?\s+\w+,\s*\[pc,\s*#(-?\d+)\]")
# A branch whose only operand is an absolute target address.
BRANCH = re.compile(r"^(b|bl|blx|bx|b[a-z]{2})(\.[nw])?\s+0x([0-9a-f]+)$")
# objdump spells an undecodable halfword/word as a data directive.
UNDECODED = re.compile(r"^\.(short|word|byte)\s+0x([0-9a-fA-F]+)$")


# --------------------------------------------------------------------------
# input tables
# --------------------------------------------------------------------------


@dataclass
class Fn:
    """One function of one translation unit, as the CSVs describe it."""

    name: str
    address: int
    size: int
    matching: bool


@dataclass
class Run:
    """A contiguous ROM range covered by one or more functions of a source."""

    start: int
    end: int
    fns: list[Fn] = field(default_factory=list)


@dataclass
class Item:
    """One emitted thing: an instruction, a pool word or a padding halfword."""

    address: int
    data: bytes
    text: str
    code: bool
    arm: bool = False
    raw: bool = False

    def render(self) -> str:
        """The assembler line, either the mnemonic or the literal encoding."""
        if not self.code:
            return _data_directive(self.data)
        if self.raw or not self.text:
            return _inst_directive(self.data, self.arm)
        return self.text


def _data_directive(data: bytes) -> str:
    """Emit data so that `as` puts a `$d` here.  Widest units first."""
    out: list[str] = []
    index = 0
    while index < len(data):
        left = len(data) - index
        if left >= 4:
            word = int.from_bytes(data[index : index + 4], "little")
            out.append(f".4byte 0x{word:08X}")
            index += 4
        elif left >= 2:
            half = int.from_bytes(data[index : index + 2], "little")
            out.append(f".2byte 0x{half:04X}")
            index += 2
        else:
            out.append(f".byte 0x{data[index]:02X}")
            index += 1
    return "\n\t".join(out)


def _inst_directive(data: bytes, arm: bool = False) -> str:
    """Emit an exact encoding that `as` still marks as code (`$t`/`$a`).

    `.inst` is the escape hatch for anything objdump cannot spell back into an
    assemblable mnemonic.  Unlike `.4byte` it does not start a `$d` run, so the
    region stays marked as code.  The `.n`/`.w` width suffixes exist only in
    Thumb; in ARM state the plain `.inst` takes the whole word.
    """
    out: list[str] = []
    index = 0
    if arm:
        while index + 4 <= len(data):
            word = int.from_bytes(data[index : index + 4], "little")
            out.append(f".inst 0x{word:08X}")
            index += 4
        if index < len(data):
            out.append(_data_directive(data[index:]))
        return "\n\t".join(out)
    while index + 4 <= len(data):
        first = int.from_bytes(data[index : index + 2], "little")
        second = int.from_bytes(data[index + 2 : index + 4], "little")
        out.append(f".inst.w 0x{(first << 16) | second:08X}")
        index += 4
    while index + 2 <= len(data):
        out.append(f".inst.n 0x{int.from_bytes(data[index:index + 2], 'little'):04X}")
        index += 2
    if index < len(data):  # cannot happen in Thumb, kept so it is never silent
        out.append(f".byte 0x{data[index]:02X}")
    return "\n\t".join(out)


def read_sources() -> dict[str, list[Fn]]:
    """Group data/c_sources.csv by source file, folding in functions.csv status."""
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        status = {row["name"]: row["status"] for row in csv.DictReader(handle)}
    grouped: dict[str, list[Fn]] = {}
    with C_SOURCES.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            size = int(row["mapped_size"] or 0)
            if size <= 0:
                continue
            grouped.setdefault(row["source"], []).append(
                Fn(
                    name=row["name"],
                    address=int(row["address"], 16),
                    size=size,
                    matching=row["matching"] == "yes"
                    and status.get(row["name"]) == "matching",
                )
            )
    for fns in grouped.values():
        fns.sort(key=lambda fn: fn.address)
    return grouped


def split_runs(fns: list[Fn]) -> list[Run]:
    """Split a source's functions into contiguous ROM runs (see RUN_GAP_LIMIT)."""
    runs: list[Run] = []
    for fn in fns:
        if runs and 0 <= fn.address - runs[-1].end <= RUN_GAP_LIMIT:
            runs[-1].end = max(runs[-1].end, fn.address + fn.size)
            runs[-1].fns.append(fn)
        else:
            runs.append(Run(fn.address, fn.address + fn.size, [fn]))
    return runs


# --------------------------------------------------------------------------
# where the code stops and the pool starts
# --------------------------------------------------------------------------


def run_tool(cmd: list[str]) -> str:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"{cmd[0]} failed: {result.stderr.strip()[:400]}")
    return result.stdout


def base_object(source: str) -> Path:
    return BASE_OBJ_DIR / (Path(source).stem + ".o")


def read_mapping(obj: Path) -> tuple[dict[str, tuple[int, int]], list[tuple[int, str]]]:
    """Return ({function: (offset, size)}, [(offset, '$t'|'$d'|'$a')]) from an object."""
    functions: dict[str, tuple[int, int]] = {}
    markers: list[tuple[int, str]] = []
    for line in run_tool([READELF, "-sW", str(obj)]).splitlines():
        parts = line.split()
        if len(parts) < 8 or not parts[0].endswith(":"):
            continue
        value, size, kind, name = parts[1], parts[2], parts[3], parts[7]
        if name in ("$t", "$d", "$a"):
            markers.append((int(value, 16), name))
        elif kind == "FUNC":
            functions[name] = (int(value, 16) & ~1, int(size))
    markers.sort()
    return functions, markers


def regions_from_object(
    fn: Fn, functions: dict[str, tuple[int, int]], markers: list[tuple[int, str]]
) -> list[tuple[int, int, str]] | None:
    """Lift our own build's `$t`/`$d` split onto the function's ROM address.

    Returns [(start, end, '$t'|'$d'|'$a')] in ROM addresses, or None when the
    object cannot answer (symbol missing, or a size our CSV disagrees with).
    """
    entry = functions.get(fn.name)
    if entry is None:
        return None
    offset, size = entry
    if size != fn.size:
        return None
    inside = [(off, kind) for off, kind in markers if offset <= off < offset + size]
    opening = [kind for off, kind in markers if off <= offset]
    if not opening:
        return None
    boundaries = [(offset, opening[-1])] + [m for m in inside if m[0] != offset]
    regions: list[tuple[int, int, str]] = []
    for index, (off, kind) in enumerate(boundaries):
        end = boundaries[index + 1][0] if index + 1 < len(boundaries) else offset + size
        regions.append((fn.address + off - offset, fn.address + end - offset, kind))
    return regions


def disassemble(data: bytes, address: int, thumb: bool, workdir: Path) -> list[tuple[int, bytes, str]]:
    """Disassemble one code region, returning [(address, bytes, mnemonic)]."""
    blob = workdir / f"r{address:08x}.bin"
    blob.write_bytes(data)
    out = run_tool([
        OBJDUMP, "-b", "binary", "-m", "arm7tdmi",
        "-M", "force-thumb" if thumb else "no-force-thumb",
        "-D", f"--adjust-vma={address:#x}", str(blob),
    ])
    blob.unlink(missing_ok=True)
    items: list[tuple[int, bytes, str]] = []
    for line in out.splitlines():
        match = DISASM_LINE.match(line)
        if not match:
            continue
        raw = bytes.fromhex(match.group(2).replace(" ", ""))
        # objdump prints Thumb halfwords big-endian-per-halfword; swap back.
        encoded = b"".join(
            raw[index : index + 2][::-1] for index in range(0, len(raw), 2)
        ) if thumb else raw[::-1]
        text = (match.group(3) or "").split("\t@")[0].split("  @")[0].strip()
        text = re.sub(r"\s+", " ", text.replace("\t", " ")).strip()
        items.append((int(match.group(1), 16), encoded, text))
    return items


def regions_from_rom(fn: Fn, data: bytes, workdir: Path, arm: bool) -> list[tuple[int, int, str]]:
    """Derive the code/pool split from the ROM alone, to a fixed point.

    One pass is not enough: until a pool word is known to be data it is
    disassembled as an instruction, and a pool word can decode into a
    pc-relative load that points at a word which is not data at all.  Iterating
    until the set stops growing settles that.
    """
    code = "$a" if arm else "$t"
    pools: set[int] = set()
    for _ in range(8):
        found = set(pools)
        for start, end, kind in _regions(fn, pools, arm):
            if kind != code:
                continue
            for address, _encoded, text in disassemble(
                data[start - fn.address : end - fn.address], start, not arm, workdir
            ):
                match = LDR_PC.search(text)
                if not match:
                    continue
                # Thumb reads the literal from (PC & ~3) + imm with PC = insn+4;
                # ARM has no alignment mask and PC = insn+8.
                target = (
                    address + 8 + int(match.group(1))
                    if arm
                    else ((address + 4) & ~3) + int(match.group(1))
                )
                if fn.address <= target and target + 4 <= fn.address + fn.size:
                    found.add(target)
        if found == pools:
            break
        pools = found
    return _regions(fn, pools, arm)


def _regions(fn: Fn, pools: set[int], arm: bool = False) -> list[tuple[int, int, str]]:
    """Turn a set of pool word addresses into alternating code/data regions."""
    end = fn.address + fn.size
    data_bytes: set[int] = set()
    for word in pools:
        data_bytes.update(range(word, min(word + 4, end)))
    code = "$a" if arm else "$t"
    regions: list[tuple[int, int, str]] = []
    address = fn.address
    while address < end:
        kind = "$d" if address in data_bytes else code
        run_end = address
        while run_end < end and (("$d" if run_end in data_bytes else code) == kind):
            run_end += 1
        regions.append((address, run_end, kind))
        address = run_end
    return regions or [(fn.address, end, code)]


def pad_before_pools(
    regions: list[tuple[int, int, str]], data: bytes, base: int, arm: bool
) -> list[tuple[int, int, str]]:
    """Move a trailing halfword of zero padding out of code and into the pool.

    agbcc aligns a literal pool to 4 and pads with 0x0000, which `as` emits as
    data.  Left inside the `$t` run it would disassemble as `movs r0, r0` and
    read as a spurious code row against a base that calls it data.
    """
    out: list[tuple[int, int, str]] = []
    for index, (start, end, kind) in enumerate(regions):
        following = regions[index + 1][2] if index + 1 < len(regions) else None
        if (
            not arm
            and kind == "$t"
            and following == "$d"
            and end - start >= 2
            and data[end - 2 - base : end - base] == b"\x00\x00"
        ):
            out.append((start, end - 2, kind))
            out.append((end - 2, end, "$d"))
        else:
            out.append((start, end, kind))
    merged: list[tuple[int, int, str]] = []
    for start, end, kind in out:
        if start == end:
            continue
        if merged and merged[-1][2] == kind and merged[-1][1] == start:
            merged[-1] = (merged[-1][0], end, kind)
        else:
            merged.append((start, end, kind))
    return merged


# --------------------------------------------------------------------------
# emission
# --------------------------------------------------------------------------


def branch_to_relative(text: str, address: int) -> str:
    """Rewrite `bl 0x8001234` as `bl . + 0x...`, which needs no relocation."""
    match = BRANCH.match(text)
    if not match or match.group(1) == "bx":
        return text
    delta = int(match.group(3), 16) - address
    sign = "+" if delta >= 0 else "-"
    return f"{match.group(1)}{match.group(2) or ''} . {sign} 0x{abs(delta):x}"


def build_items(
    fn: Fn, regions: list[tuple[int, int, str]], data: bytes, workdir: Path, arm: bool
) -> list[Item]:
    """Turn one function's regions into the list of lines that will be emitted."""
    items: list[Item] = []
    for start, end, kind in regions:
        chunk = data[start - fn.address : end - fn.address]
        if kind == "$d":
            items.append(Item(start, chunk, "", code=False))
            continue
        thumb = kind == "$t"
        for address, encoded, text in disassemble(chunk, start, thumb, workdir):
            undecoded = UNDECODED.match(text)
            items.append(
                Item(
                    address,
                    encoded,
                    "" if undecoded else branch_to_relative(text, address),
                    code=True,
                    arm=not thumb,
                )
            )
    return items


def render(
    source: str,
    runs: list[Run],
    items: dict[str, list[Item]],
    pads: dict[str, list[Item]],
    arm: bool,
) -> tuple[str, list[int | None]]:
    """Write the whole `.s` and remember which item each line came from."""
    lines: list[str] = []
    owner: list[int | None] = []
    flat: list[Item] = []

    def put(text: str, index: int | None = None) -> None:
        for piece in text.split("\n"):
            lines.append(piece)
            owner.append(index)

    put("@ Generated by tools/gen_expected.py from baserom.gba -- do not edit.")
    put(f"@ Source of truth for {source}.")
    put("\t.syntax unified")
    put(f"\t.section {SECTION_NAME}, \"ax\", %progbits")
    put("\t.arm" if arm else "\t.thumb")
    for run in runs:
        for fn in run.fns:
            put("")
            put(f"\t.global {fn.name}")
            put(f"\t.type {fn.name}, %function")
            if not arm:
                put("\t.thumb_func")
            put(f"{fn.name}:")
            for item in items[fn.name]:
                index = len(flat)
                flat.append(item)
                put("\t" + item.render().replace("\n\t", "\n\t"), index)
            put(f"\t.size {fn.name}, . - {fn.name}")
            # Inter-function alignment padding belongs to the run's ROM range
            # but not to the symbol, so it is emitted after `.size`.
            for item in pads.get(fn.name, []):
                index = len(flat)
                flat.append(item)
                put("\t" + item.render(), index)
    return "\n".join(lines) + "\n", owner


# --------------------------------------------------------------------------
# per-source pipeline
# --------------------------------------------------------------------------


@dataclass
class Result:
    source: str
    ok: bool
    reason: str = ""
    obj: Path | None = None
    raw_items: int = 0
    total_items: int = 0


def object_path(source: str, outdir: Path) -> Path:
    return outdir / Path(source).with_suffix(".o")


def generate(source: str, fns: list[Fn], rom: bytes, outdir: Path) -> Result:
    """Build, assemble and verify one translation unit's target object."""
    try:
        return _generate(source, fns, rom, outdir)
    except Exception as error:  # a bad unit must not take the run down
        return Result(source, False, f"{type(error).__name__}: {error}")


def _generate(source: str, fns: list[Fn], rom: bytes, outdir: Path) -> Result:
    path = ROOT / source
    arm = path.exists() and ARM_MARKER in path.read_text(encoding="utf-8")
    runs = split_runs(fns)

    functions: dict[str, tuple[int, int]] = {}
    markers: list[tuple[int, str]] = []
    obj = base_object(source)
    if obj.exists():
        functions, markers = read_mapping(obj)

    workdir = Path(tempfile.mkdtemp(prefix="gen_expected_"))
    try:
        items: dict[str, list[Item]] = {}
        pads: dict[str, list[Item]] = {}
        for run in runs:
            chunk = rom[run.start - ROM_BASE : run.end - ROM_BASE]
            if len(chunk) != run.end - run.start:
                return Result(source, False, "run falls outside the ROM")
            for index, fn in enumerate(run.fns):
                body = chunk[fn.address - run.start : fn.address - run.start + fn.size]
                regions = None
                if fn.matching:
                    regions = regions_from_object(fn, functions, markers)
                if regions is None:
                    regions = regions_from_rom(fn, body, workdir, arm)
                regions = pad_before_pools(regions, body, fn.address, arm)
                fn_items = build_items(fn, regions, body, workdir, arm)
                # Inter-function padding is attached to the function it follows
                # so that the run stays a single uninterrupted ROM range.
                nxt = run.fns[index + 1].address if index + 1 < len(run.fns) else run.end
                tail = fn.address + fn.size
                if nxt > tail:
                    pads[fn.name] = [
                        Item(tail, chunk[tail - run.start : nxt - run.start], "", code=False)
                    ]
                items[fn.name] = fn_items

        # THE contract.  This is read straight out of the ROM, NOT rebuilt from
        # the items: if the items had been mis-sliced, an expectation derived
        # from them would carry the same mistake and the comparison would be
        # comparing the object against itself.
        expected = b"".join(
            rom[run.start - ROM_BASE : run.end - ROM_BASE] for run in runs
        )

        source_file = workdir / "unit.s"
        obj_file = workdir / "unit.o"
        bin_file = workdir / "unit.bin"
        raw_count = 0
        # Assemble, compare, and demote whatever byte disagrees to its literal
        # encoding.  The first differing offset strictly increases each round,
        # because a demoted item is exact, so this terminates.
        for _ in range(256):
            text, owner = render(source, runs, items, pads, arm)
            source_file.write_text(text, encoding="utf-8")
            result = subprocess.run(
                [AS, *AS_FLAGS, "-o", str(obj_file), str(source_file)],
                capture_output=True, text=True,
            )
            flat = [
                item
                for run in runs
                for fn in run.fns
                for item in items[fn.name] + pads.get(fn.name, [])
            ]
            if result.returncode != 0:
                bad = _items_from_errors(result.stderr, owner)
                if not bad or all(flat[i].raw or not flat[i].code for i in bad):
                    return Result(source, False, f"assembly failed: {result.stderr.strip()[:200]}")
                for index in bad:
                    flat[index].raw = True
                    raw_count += 1
                continue
            run_tool([OBJCOPY, "-O", "binary", f"--only-section={SECTION_NAME}",
                      str(obj_file), str(bin_file)])
            got = bin_file.read_bytes()
            if got == expected:
                target = object_path(source, outdir)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(obj_file, target)
                # The `.s` is kept beside the object: it is the audit trail for
                # the code/pool split and costs nothing to look at.
                target.with_suffix(".s").write_text(text, encoding="utf-8")
                return Result(source, True, obj=target,
                              raw_items=raw_count, total_items=len(flat))
            index = _first_difference(got, expected)
            offset, victim = 0, None
            for position, item in enumerate(flat):
                if offset <= index < offset + len(item.data):
                    victim = position
                    break
                offset += len(item.data)
            if victim is None or flat[victim].raw or not flat[victim].code:
                # Nothing left to demote at that offset: fall back to emitting
                # every instruction literally, which is byte-exact by
                # construction, and let the comparison below judge it.
                if all(item.raw or not item.code for item in flat):
                    return Result(source, False,
                                  f"bytes still differ at +0x{index:X} with literal encodings")
                for item in flat:
                    if item.code and not item.raw:
                        item.raw = True
                        raw_count += 1
                continue
            flat[victim].raw = True
            raw_count += 1
        return Result(source, False, "did not converge after 256 assembler rounds")
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


def _first_difference(got: bytes, expected: bytes) -> int:
    for index, (left, right) in enumerate(zip(got, expected)):
        if left != right:
            return index
    return min(len(got), len(expected))


def _items_from_errors(stderr: str, owner: list[int | None]) -> list[int]:
    """Map `as` diagnostics back to the items whose lines produced them."""
    bad: list[int] = []
    for match in re.finditer(r"\.s:(\d+):\s*(?:Error|Warning)", stderr):
        line = int(match.group(1)) - 1
        if 0 <= line < len(owner) and owner[line] is not None:
            bad.append(owner[line])
    return sorted(set(bad))


# --------------------------------------------------------------------------
# entry point
# --------------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--only", default="",
                        help="only sources whose path contains this substring")
    parser.add_argument("-o", "--outdir", type=Path, default=EXPECTED_DIR,
                        help="where the target objects go (default: expected/)")
    parser.add_argument("-j", "--jobs", type=int, default=8, help="parallel workers")
    parser.add_argument("--strict", action="store_true",
                        help="exit non-zero if any source is skipped")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="one line per source instead of only the skips")
    args = parser.parse_args()

    for tool in (AS, OBJCOPY, OBJDUMP, READELF):
        if shutil.which(tool) is None:
            print(f"{tool} is not on PATH; run tools/verify_toolchain.py", file=sys.stderr)
            return 2
    if not ROM_PATH.exists():
        print("baserom.gba is missing. First run: make prepare-rom ROM_ZIP=...", file=sys.stderr)
        return 2

    rom = ROM_PATH.read_bytes()
    grouped = read_sources()
    selected = {src: fns for src, fns in grouped.items() if args.only in src}
    if not selected:
        print(f"no source in {C_SOURCES} matches --only {args.only!r}", file=sys.stderr)
        return 2

    results: list[Result] = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = {
            pool.submit(generate, src, fns, rom, args.outdir): src
            for src, fns in sorted(selected.items())
        }
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            results.append(result)
            if not result.ok:
                print(f"SKIP {result.source}: {result.reason}", file=sys.stderr)
            elif args.verbose:
                note = f" ({result.raw_items}/{result.total_items} literal)" if result.raw_items else ""
                print(f"OK   {result.source}{note}")

    written = [r for r in results if r.ok]
    skipped = [r for r in results if not r.ok]
    print(f"\n{len(written)} objects written and verified against the ROM, "
          f"{len(skipped)} skipped, out of {len(selected)} sources.")
    if skipped:
        reasons: dict[str, int] = {}
        for result in skipped:
            reasons[result.reason.split(":")[0]] = reasons.get(result.reason.split(":")[0], 0) + 1
        for reason, count in sorted(reasons.items(), key=lambda kv: -kv[1]):
            print(f"  {count:4d}  {reason}")
    if not written:
        return 1
    return 1 if (args.strict and skipped) else 0


if __name__ == "__main__":
    raise SystemExit(main())
