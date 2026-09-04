#!/usr/bin/env python3
"""agbcc'nin register dagitim tablosunu okur.

agbcc (GCC 2.8) `-dg` ile global dagitim dokumu uretiyor ve dokumde
dagiticinin KENDI oncelik listesi yazili: her pseudo-register icin
`refs` (referans sayisi) ve `live_length` (omur uzunlugu).

Sira su formulle belirleniyor (dokume karsi dogrulandi):
    oncelik = floor_log2(refs) * refs / live_length

Bu, canli kumeyi TAHMIN ETMEYI gereksiz kilar: hangi degerin kac
referansi oldugunu ve ne kadar yasadigini derleyici sOyluyor. Bir
fonksiyon ROM'dan fazla callee-saved register istiyorsa, tabloyu okuyup
hangi pseudo'nun fazladan yer kapladigi gorulebilir.

Kullanim: python3 tools/dump_alloc.py <kaynak.c> [fonksiyon]
"""
import re
import subprocess
import sys
import tempfile
from math import floor, log2
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AGBCC = ROOT / "tools/agbcc/bin/old_agbcc"
FLAGS = ["-mthumb-interwork", "-O2", "-fhex-asm"]


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    source = Path(sys.argv[1])
    want = sys.argv[2] if len(sys.argv) > 2 else None

    with tempfile.TemporaryDirectory() as d:
        work = Path(d)
        pre = work / "in.i"
        result = subprocess.run(
            ["cpp", "-nostdinc", "-undef", f"-I{ROOT / 'include'}", str(source)],
            capture_output=True, text=True)
        if result.returncode:
            sys.exit(f"cpp basarisiz:\n{result.stderr[:400]}")
        pre.write_text(result.stdout)
        result = subprocess.run(
            [str(AGBCC), *FLAGS, "-dg", "-o", str(work / "out.s"), str(pre)],
            capture_output=True, text=True)
        if result.returncode:
            sys.exit(f"agbcc basarisiz:\n{result.stderr[:400]}")
        dump = (work / "in.i.greg")
        if not dump.exists():
            sys.exit("dokum uretilmedi")
        text = dump.read_text()

    for block in text.split(";; Function ")[1:]:
        name = block.split("\n", 1)[0].strip()
        if want and name != want:
            continue
        rows = re.findall(
            r"Register (\d+), refs = (\d+), live_length = (\d+)", block)
        spills = len(re.findall(r"^Spilling for insn", block, re.M))
        print(f"\n{name}: {len(rows)} pseudo-register, {spills} spill")
        if not rows:
            continue
        print(f"  {'pseudo':>7} {'refs':>5} {'omur':>5} {'oncelik':>9}")
        for reg, refs, length in rows:
            r, ln = int(refs), int(length)
            pri = (floor(log2(r)) * r / ln) if r > 1 and ln else 0.0
            print(f"  {reg:>7} {r:>5} {ln:>5} {pri:>9.3f}")


if __name__ == "__main__":
    main()
