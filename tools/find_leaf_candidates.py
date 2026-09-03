#!/usr/bin/env python3
"""Henuz eslesmemis fonksiyonlar arasindan en kolay hedefleri bulur.

Bir fonksiyon "yaprak" ise (hic `bl` icermiyorsa) hicbir dis sembole
bagimli degildir ve tek basina yazilip dogrulanabilir. Yaprak olmayanlar
icin cagirdigi her hedefin data/functions.csv'de adres karsiligi olmali.

Siralama: once yapraklar, sonra boyuta gore. Kucuk yapraklar en ucuz
kazanclardir.

Kullanim:  python3 tools/find_leaf_candidates.py [--limit N] [--max-size N]
"""
import csv
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000

BL = re.compile(r"\bbl\s+0x([0-9a-f]+)")
EPILOGUE = re.compile(r"\b(pop|add\s+sp)\b")


def is_fragment(out: str) -> bool:
    """Aday gercek bir fonksiyon mu, yoksa buyuk bir fonksiyonun kuyrugu mu.

    Ghidra atlama tablolarinda cozumleyemeyip fonksiyonlari ortadan
    kesiyor; ortaya cikan parcalar `push` ile baslamadiklari halde `pop`
    veya `add sp` iceriyor. Bunlar tek baslarina yazilamaz: prologlari
    baska bir adreste.
    """
    body = [l for l in out.splitlines() if re.match(r"\s*[0-9a-f]+:\s", l)]
    if not body:
        return False
    starts_with_push = "push" in body[0]
    return not starts_with_push and any(EPILOGUE.search(l) for l in body)


def main() -> None:
    limit = 30
    max_size = 200
    for arg in sys.argv[1:]:
        if arg.startswith("--limit="):
            limit = int(arg.split("=", 1)[1])
        elif arg.startswith("--max-size="):
            max_size = int(arg.split("=", 1)[1])

    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    known = {int(r["address"], 16) for r in rows}
    pending = [
        r for r in rows
        if r["status"] == "candidate" and r["size"].strip()
        and 8 <= int(r["size"]) <= max_size
    ]

    work = Path(tempfile.mkdtemp())
    results = []
    fragments = []
    for row in pending:
        address = int(row["address"], 16)
        size = int(row["size"])
        start = address - ROM_BASE
        slice_path = work / "f.bin"
        slice_path.write_bytes(rom[start:start + size])
        out = subprocess.run([
            "arm-none-eabi-objdump", "-b", "binary", "-m", "arm7tdmi",
            "-M", "force-thumb", "-D", f"--adjust-vma={address:#x}",
            str(slice_path),
        ], capture_output=True, text=True).stdout
        if is_fragment(out):
            fragments.append(row)
            continue
        targets = {int(m, 16) for m in BL.findall(out)}
        unresolved = [t for t in targets if t not in known]
        results.append((len(targets), len(unresolved), size, row, sorted(targets)))

    # Once yapraklar, sonra cozulebilir cagrilar, sonra boyut
    results.sort(key=lambda r: (r[0] > 0, r[1], r[2]))

    print(f"{'adres':12} {'boyut':>5} {'cagri':>5} {'cozulemez':>9}  modul")
    print("-" * 62)
    for calls, unresolved, size, row, targets in results[:limit]:
        tag = "YAPRAK" if calls == 0 else f"{calls} cagri"
        print(f"{row['address']} {size:>5} {tag:>10} {unresolved:>7}   {row['module']}")
    leaves = sum(1 for r in results if r[0] == 0)
    print("-" * 62)
    print(f"{len(results)} aday incelendi (8-{max_size} byte): "
          f"{leaves} yaprak, {len(results) - leaves} cagri iceren")
    if fragments:
        print(f"{len(fragments)} aday elendi: `push` ile baslamadiklari halde "
              f"`pop`/`add sp` iceriyorlar, yani buyuk fonksiyonlarin kuyruklari "
              f"(Ghidra atlama tablosunda kesmis). Ornek: "
              + ", ".join(r["address"] for r in fragments[:5]))


if __name__ == "__main__":
    main()
