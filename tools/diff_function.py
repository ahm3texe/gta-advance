#!/usr/bin/env python3
"""Bir fonksiyonun ROM halini derlenmis halinin yaninda gosterir.

Kullanim:
    python3 tools/diff_function.py src/save/save_helpers.c WriteU16LE

Sol sutun ROM'daki gercek kod, sag sutun senin C'nden uretilen kod.
Farkli satirlar isaretlenir. Eslesmeyen bir fonksiyonu duzeltirken
"nerede sapiyor" sorusunun cevabi budur.
"""
import csv
import difflib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AGBCC_DIR = ROOT / "tools/agbcc/bin"
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
BUILD = ROOT / "build/cmatch"
DEFAULT_CC = "old_agbcc"
CC1FLAGS = ["-mthumb-interwork", "-O2", "-fhex-asm"]
ROM_BASE = 0x08000000

GREEN, RED, DIM, RESET = "\033[32m", "\033[31m", "\033[2m", "\033[0m"


def run(cmd: list[str], stdout: Path | None = None) -> str:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"HATA: {' '.join(cmd)}\n{result.stderr.strip()}")
    if stdout is not None:
        stdout.write_text(result.stdout, encoding="utf-8")
    return result.stdout


def disassemble(blob: bytes, base: int) -> list[str]:
    """Thumb kod blogunu komut listesine cevirir."""
    raw = BUILD / "dis.bin"
    raw.write_bytes(blob)
    out = run([
        "arm-none-eabi-objdump", "-b", "binary", "-m", "arm7tdmi",
        "-M", "force-thumb", "-D", f"--adjust-vma={base:#x}", str(raw),
    ])
    lines = []
    for line in out.splitlines():
        match = re.match(r"\s*([0-9a-f]+):\s+([0-9a-f ]+)\t(.*)", line)
        if match:
            lines.append(re.sub(r"\s+", " ", match.group(3)).strip())
    return lines


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    compiler = next(
        (a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--cc=")),
        DEFAULT_CC,
    )
    if len(args) != 2:
        sys.exit(__doc__)
    source, target = Path(args[0]), args[1]

    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = {r["name"]: r for r in csv.DictReader(handle)}
    if target not in rows:
        sys.exit(f"{target} data/functions.csv icinde yok")
    address = int(rows[target]["address"], 16)
    rom_size = int(rows[target]["size"] or 0)

    BUILD.mkdir(parents=True, exist_ok=True)
    stem = BUILD / source.stem
    run(["cpp", "-nostdinc", "-undef", str(source)], Path(f"{stem}.i"))
    run([str(AGBCC_DIR / compiler), *CC1FLAGS, "-o", f"{stem}.s", f"{stem}.i"])
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.o", f"{stem}.s"])
    run(["arm-none-eabi-objcopy", "-O", "binary", f"{stem}.o", f"{stem}.bin"])

    nm = run(["arm-none-eabi-nm", "-S", f"{stem}.o"])
    symbols = {
        p[3]: (int(p[0], 16), int(p[1], 16))
        for p in (l.split() for l in nm.splitlines())
        if len(p) == 4 and p[2] in "tT"
    }
    if target not in symbols:
        sys.exit(f"{target} {source} icinde tanimli degil")

    offset, size = symbols[target]
    mine = Path(f"{stem}.bin").read_bytes()[offset:offset + size]
    # ROM tarafi kendi gercek boyutuyla okunur; boyutlar farkli olabilir.
    start = address - ROM_BASE
    theirs = ROM.read_bytes()[start:start + (rom_size or size)]

    rom_asm = disassemble(theirs, address)
    our_asm = disassemble(mine, address)

    print(f"{target}  @ 0x{address:08X}  "
          f"ROM {len(theirs)} byte / seninki {len(mine)} byte  [{compiler}]")
    print(f"{'ROM (hedef)':38}   senin C çıktın")
    print("-" * 78)

    same = 0
    matcher = difflib.SequenceMatcher(None, rom_asm, our_asm, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for line in rom_asm[i1:i2]:
                same += 1
                print(f"{DIM}{line:38} = {line}{RESET}")
        elif tag == "replace":
            for k in range(max(i2 - i1, j2 - j1)):
                left = rom_asm[i1 + k] if i1 + k < i2 else ""
                right = our_asm[j1 + k] if j1 + k < j2 else ""
                print(f"{RED}{left:38} ≠ {right}{RESET}")
        elif tag == "delete":
            for line in rom_asm[i1:i2]:
                print(f"{RED}{line:38} ← sende YOK{RESET}")
        elif tag == "insert":
            for line in our_asm[j1:j2]:
                print(f"{RED}{'ROM da YOK':38} → {line}{RESET}")

    print("-" * 78)
    total = max(len(rom_asm), len(our_asm))
    if mine == theirs:
        print(f"{GREEN}BYTE-MATCHING{RESET}")
    else:
        print(f"{same}/{total} komut ayni, {total - same} farkli")
    sys.exit(0 if mine == theirs else 1)


if __name__ == "__main__":
    main()
