#!/usr/bin/env python3
"""Sweep STRUCTURAL VARIANTS over a C source and compare against the ROM.

Why
---
On near misses, both manual attempts and decomp-permuter stall, but a
SYSTEMATIC sweep solves it. Measured (2026-09-05, EitherInRange 8/44):

  - 5040 declaration orders tried by hand -> none got below 8
  - decomp-permuter ran 20000 iterations -> plateaued at score 205
  - this tool's "hoist the comparison constant into a local" sweep -> 0/44

The permuter makes RANDOM local mutations; this tool tries ALL subsets of
SPECIFIC mechanisms. The two are good for different things: the permuter is
broad and blind, this tool is narrow and exhaustive.

SCOPE LIMIT -- MEASURED, DO NOT OVERSTATE
-----------------------------------------
This tool sweeps ONLY the mechanisms it knows. On the three current near
misses (2026-09-05) it found nothing:

    ClearTextArea    1/132   -> 17 variants, no result
    CleanupAreaTiles 7/88    ->  2 variants, no result
    FUN_08045AA4    12/540   ->  2 variants, no result

The reason: all three are blocked by LOAD/STORE REGISTER ASSIGNMENT, not by
comparison canonicalization. For that class there is NO known source-level
lever. The obvious first idea, a DECLARATION ORDER sweep, is also empty: an
earlier model exhausted 5040 orderings for ClearTextArea and none improved
anything. A sweep cannot be written for an unknown mechanism.

So this tool is right and fast for rule 44 type (comparison constant) misses;
for register allocation misses it is NO CURE. If a new mechanism is
discovered, it should be added to the TRANSFORMS dictionary.

Usage
-----
    python3 tools/sweep_variants.py <source.c> <FunctionName>
    python3 tools/sweep_variants.py <source.c> <Name> --only hoist_cmp
    python3 tools/sweep_variants.py <source.c> <Name> --max-sites 8 --keep

The source file is NOT MODIFIED; the best variant is written only with --keep.
"""
import argparse
import itertools
import re
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from verify_c_function import (  # noqa: E402
    compile_and_link, DEFAULT_CC, rom_bytes, ROM_BASE,
)

# ------------------------------------------------------------------- helpers

def function_row(name):
    import csv
    for r in csv.DictReader((ROOT / "data/functions.csv").open()):
        if r["name"] == name:
            return int(r["address"], 16), int(r["size"] or 0)
    sys.exit(f"{name} is not in data/functions.csv")


def body_span(text, name):
    """The [start, end) span of the function body, including the opening brace."""
    m = re.search(rf"\b{re.escape(name)}\s*\([^;{{]*\)\s*\{{", text)
    if not m:
        return None
    i = m.end() - 1
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return i, j + 1
    return None


# ------------------------------------------------------------ transformations
# Each transform: (name, body) -> [(site_description, apply_fn), ...]
# apply_fn(body) -> new body. Applications run BACK TO FRONT so that offsets
# do not shift.

CMP_LITERAL = re.compile(r"(\b[A-Za-z_]\w*(?:->|\.)?\w*)\s*(<=|>=|<|>|==|!=)\s*(\d+)\b")


def sites_hoist_cmp(body, decl_anchor):
    """Rule 44: hoist the comparison constant into a local variable.

    agbcc canonicalizes `x < 15` to `x <= 14`; with the constant in a variable,
    canonicalization is skipped and the constant is still emitted as an
    immediate.
    """
    out = []
    for k, m in enumerate(CMP_LITERAL.finditer(body)):
        var, op, lit = m.group(1), m.group(2), m.group(3)
        s, e = m.span()
        name = f"swp{k}"

        def apply(b, s=s, e=e, var=var, op=op, lit=lit, name=name):
            decl = f"    s32 {name} = {lit};\n"
            return (b[:s] + f"{var} {op} {name}" + b[e:], decl)

        out.append((f"hoist_cmp[{var}{op}{lit}]", apply))
    return out


def sites_flip_cmp(body, decl_anchor):
    """Swap the comparison operands (a<b -> b>a). This can change the emission
    order and therefore the pool/register assignment."""
    FLIP = {"<": ">", ">": "<", "<=": ">=", ">=": "<=", "==": "==", "!=": "!="}
    out = []
    for k, m in enumerate(CMP_LITERAL.finditer(body)):
        var, op, lit = m.group(1), m.group(2), m.group(3)
        s, e = m.span()

        def apply(b, s=s, e=e, var=var, op=op, lit=lit):
            return (b[:s] + f"{lit} {FLIP[op]} {var}" + b[e:], "")

        out.append((f"flip_cmp[{var}{op}{lit}]", apply))
    return out


ASSIGN_EXPR = re.compile(r"^(\s+)([A-Za-z_]\w*)\s*=\s*([^;=][^;]*);\s*$", re.M)


def sites_split_assign(body, decl_anchor):
    """`a = X op Y;` -> split into an intermediate variable. Changes the number
    of live values and therefore the allocation."""
    out = []
    for k, m in enumerate(ASSIGN_EXPR.finditer(body)):
        indent, lhs, rhs = m.group(1), m.group(2), m.group(3).strip()
        if len(rhs) < 8 or "(" in rhs.split()[0:1]:
            continue
        parts = re.split(r"\s(\+|-|\*|>>|<<|\||&)\s", rhs, maxsplit=1)
        if len(parts) != 3:
            continue
        s, e = m.span()
        name = f"swt{k}"

        def apply(b, s=s, e=e, indent=indent, lhs=lhs, parts=parts, name=name):
            new = (f"{indent}{name} = {parts[0]};\n"
                   f"{indent}{lhs} = {name} {parts[1]} {parts[2]};\n")
            return (b[:s] + new + b[e:], f"    s32 {name};\n")

        out.append((f"split_assign[{lhs}]", apply))
    return out


TRANSFORMS = {
    "hoist_cmp": sites_hoist_cmp,
    "flip_cmp": sites_flip_cmp,
    "split_assign": sites_split_assign,
}


# --------------------------------------------------------------------- sweep

def score(path, name, addr, size, rom):
    try:
        blob, layout, _ = compile_and_link(path, DEFAULT_CC)
    except SystemExit:
        return None, None
    if name not in layout:
        return None, None
    off, n = layout[name]
    got = blob[off:off + n]
    want = rom[addr - ROM_BASE:addr - ROM_BASE + size]
    return sum(a != b for a, b in zip(got, want)) + abs(n - size), n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("function")
    ap.add_argument("--only", action="append",
                    help=f"only this transform ({', '.join(TRANSFORMS)})")
    ap.add_argument("--max-sites", type=int, default=10,
                    help="max sites per transform (2^n combinations)")
    ap.add_argument("--keep", action="store_true",
                    help="WRITE the best variant back to the source file")
    a = ap.parse_args()

    src = Path(a.source)
    addr, size = function_row(a.function)
    rom = rom_bytes()
    original = src.read_text()
    span = body_span(original, a.function)
    if not span:
        sys.exit(f"the body of {a.function} was not found in {src}")
    bstart, bend = span
    head, body, tail = original[:bstart], original[bstart:bend], original[bend:]
    decl_anchor = body.index("\n") + 1   # the line after the opening brace

    base_score, base_size = score(src, a.function, addr, size, rom)
    if base_score is None:
        sys.exit("the base version did not compile")
    print(f"base: {base_size}/{size} bytes, difference {base_score}")
    if base_score == 0:
        print("already matching"); return 0

    names = a.only or list(TRANSFORMS)
    best = (base_score, None, "baseline")
    tried = 0
    work = Path(tempfile.mkdtemp(prefix="sweep_")) / src.name

    for tname in names:
        sites = TRANSFORMS[tname](body, decl_anchor)
        if not sites:
            print(f"  {tname}: no sites"); continue
        if len(sites) > a.max_sites:
            print(f"  {tname}: {len(sites)} sites -> limited to the first {a.max_sites} "
                  f"(KAPSAM DUSURULDU)")
            sites = sites[:a.max_sites]
        n = len(sites)
        print(f"  {tname}: {n} sites, {2**n} combinations")
        for r in range(1, n + 1):
            for combo in itertools.combinations(range(n), r):
                b = body
                decls = ""
                for k in sorted(combo, reverse=True):   # back to front
                    b, d = sites[k][1](b)
                    decls += d
                if decls:
                    b = b[:decl_anchor] + decls + b[decl_anchor:]
                work.write_text(head + b + tail)
                sc, sz = score(work, a.function, addr, size, rom)
                tried += 1
                if sc is None:
                    continue
                if sc < best[0]:
                    best = (sc, head + b + tail, f"{tname}{combo}")
                    print(f"    new best: difference {sc} size {sz} <- {tname}{list(combo)}")
                if sc == 0:
                    print(f"\n*** MATCHED *** {tname}{list(combo)}  ({tried} variants tried)")
                    if a.keep:
                        src.write_text(best[1]); print(f"    written: {src}")
                    else:
                        out = src.with_suffix(".match.c")
                        out.write_text(best[1]); print(f"    saved: {out}")
                    return 0

    print(f"\n{tried} variants tried; best difference {best[0]} ({best[2]})")
    if best[1] and best[0] < base_score:
        out = src.with_suffix(".best.c")
        out.write_text(best[1]); print(f"best variant: {out}")
        if a.keep:
            src.write_text(best[1]); print(f"source updated: {src}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
