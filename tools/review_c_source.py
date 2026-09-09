#!/usr/bin/env python3
"""Check C sources for readability.

A byte match is not enough on its own: the goal is readable source. This tool
catches the "matches but unreadable" case -- inline assembly, variable names left
over from Ghidra's output, bare address constants, a missing header comment.

Usage:  python3 tools/review_c_source.py [file.c ...]
        (with no arguments it looks at every .c file under src/)
"""
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RED, YELLOW, GREEN, RESET = "\033[31m", "\033[33m", "\033[32m", "\033[0m"

# Naming specific to Ghidra decompiler output
GHIDRA_NAMES = re.compile(
    r"\b(uVar\d+|iVar\d+|bVar\d+|cVar\d+|fVar\d+|puVar\d+|piVar\d+"
    r"|param_\d+|local_[0-9a-f]+|DAT_[0-9a-f]+|unaff_\w+|in_\w+)\b"
)
INLINE_ASM = re.compile(r"\b(__asm__|asm\s*\(|__attribute__\s*\(\s*\(\s*naked)")
# register T *p asm("r4") -- byte'lari tutturur ama nedenini gizler.
REGISTER_PIN = re.compile(r"\bregister\b[^;\n]*\basm\s*\(")
# A bare hardware/RAM address outside a comment or #define
BARE_ADDRESS = re.compile(r"0x0[2-8][0-9A-Fa-f]{6}")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def renamed_symbols() -> dict[str, str]:
    """Addresses that are no longer FUN_: if the old name stays in the source, the build breaks silently."""
    path = ROOT / "data/functions.csv"
    if not path.exists():
        return {}
    with path.open(newline="", encoding="utf-8") as handle:
        out = {}
        for row in csv.DictReader(handle):
            address = int(row["address"], 16)
            legacy = f"FUN_{address:08x}"
            if row["name"] != legacy:
                out[legacy] = row["name"]
        return out


def review(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    code = strip_comments(text)
    problems = []

    if not text.lstrip().startswith("/*"):
        problems.append("no header comment")

    for match in sorted(set(GHIDRA_NAMES.findall(code))):
        problems.append(f"Ghidra kalintisi degisken adi: {match}")

    if REGISTER_PIN.search(code):
        problems.append("acik register baglamasi (register ... asm(\"rN\")) -- "
                        "eslesmeyi zorlar ama nedenini gizler; docs/WORKFLOW.md 6")
    elif INLINE_ASM.search(code):
        problems.append("contains inline assembly -- defeats the purpose of moving to C")

    define_lines = {
        line for line in code.splitlines() if line.lstrip().startswith("#define")
    }
    for line in code.splitlines():
        if line in define_lines:
            continue
        for addr in BARE_ADDRESS.findall(line):
            problems.append(f"bare address outside a #define {addr}: {line.strip()[:60]}")

    for legacy, current in renamed_symbols().items():
        if legacy in code:
            problems.append(f"stale symbol name {legacy}; it is now {current}")

    comment_chars = len(text) - len(code)
    if len(code) and comment_chars / len(text) < 0.05:
        problems.append(
            f"yorum orani cok dusuk (%{100 * comment_chars / len(text):.1f})"
        )
    return problems


def main() -> None:
    targets = [Path(a) for a in sys.argv[1:]] or sorted((ROOT / "src").rglob("*.c"))
    worst = 0
    for path in targets:
        problems = review(path)
        try:
            rel = path.relative_to(ROOT)
        except ValueError:
            rel = path
        if not problems:
            print(f"{GREEN}CLEAN{RESET}  {rel}")
            continue
        worst = 1
        print(f"{YELLOW}WARN{RESET}   {rel}")
        for problem in problems[:8]:
            print(f"         - {problem}")
        if len(problems) > 8:
            print(f"         ... ve {len(problems) - 8} uyari daha")
    sys.exit(worst)


if __name__ == "__main__":
    main()
