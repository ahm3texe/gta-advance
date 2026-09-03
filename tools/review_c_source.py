#!/usr/bin/env python3
"""C kaynaklarini okunabilirlik acisindan denetler.

Byte eslesmesi tek basina yetmez: amac okunabilir kaynak uretmek. Bu arac
"eslesiyor ama okunamiyor" durumunu yakalar -- inline assembly, Ghidra
ciktisindan kalan degisken adlari, cikplak adres sabitleri, eksik baslik.

Kullanim:  python3 tools/review_c_source.py [dosya.c ...]
           (argumansiz calisirsa src/ altindaki tum .c dosyalarina bakar)
"""
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RED, YELLOW, GREEN, RESET = "\033[31m", "\033[33m", "\033[32m", "\033[0m"

# Ghidra decompiler ciktisina ozgu adlandirmalar
GHIDRA_NAMES = re.compile(
    r"\b(uVar\d+|iVar\d+|bVar\d+|cVar\d+|fVar\d+|puVar\d+|piVar\d+"
    r"|param_\d+|local_[0-9a-f]+|DAT_[0-9a-f]+|unaff_\w+|in_\w+)\b"
)
INLINE_ASM = re.compile(r"\b(__asm__|asm\s*\(|__attribute__\s*\(\s*\(\s*naked)")
# Yorum veya #define disinda gecen ciplak donanim/RAM adresi
BARE_ADDRESS = re.compile(r"0x0[2-8][0-9A-Fa-f]{6}")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def renamed_symbols() -> dict[str, str]:
    """Artik FUN_ olmayan adresler: eski adi kaynakta kalirsa build sessizce bozulur."""
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
        problems.append("baslik yorumu yok")

    for match in sorted(set(GHIDRA_NAMES.findall(code))):
        problems.append(f"Ghidra kalintisi degisken adi: {match}")

    if INLINE_ASM.search(code):
        problems.append("inline assembly iceriyor -- C'ye tasima amacini bozar")

    define_lines = {
        line for line in code.splitlines() if line.lstrip().startswith("#define")
    }
    for line in code.splitlines():
        if line in define_lines:
            continue
        for addr in BARE_ADDRESS.findall(line):
            problems.append(f"#define disinda ciplak adres {addr}: {line.strip()[:60]}")

    for legacy, current in renamed_symbols().items():
        if legacy in code:
            problems.append(f"bayat sembol adi {legacy}; artik {current}")

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
        rel = path.relative_to(ROOT) if path.is_absolute() else path
        if not problems:
            print(f"{GREEN}TEMIZ{RESET}  {rel}")
            continue
        worst = 1
        print(f"{YELLOW}UYARI{RESET}  {rel}")
        for problem in problems[:8]:
            print(f"         - {problem}")
        if len(problems) > 8:
            print(f"         ... ve {len(problems) - 8} uyari daha")
    sys.exit(worst)


if __name__ == "__main__":
    main()
