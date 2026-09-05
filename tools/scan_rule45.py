#!/usr/bin/env python3
"""Kural 45'in gecerli oldugu fonksiyonlari Ghidra ciktisindan tespit eder.

Kural 45 (docs/COMPILER.md): uzun if/else zincirlerinde birden cok dalin
govdesi ayniysa agbcc onlari capraz atlamayla birlestirir ve bir dalin kodu
tumuyle kaybolur.  ROM'da o dallar ayri fiziksel kopyalarsa, orijinal
kaynakta her dalin KENDI yerel degiskenleri vardir.

Bu arac iki isareti sayar:

  tekrar   Ghidra ciktisinda birbirinin ayni olan (adlar disinda) ard arda
           gelen ifade bloklari.  Yuksekse birlesme riski yuksektir.
  yerel    Ghidra'nin urettigi ayri `local_XX` sayisi.  Ghidra bunlari
           dogru gosteriyor; cok sayida yerel, dallarin ayri yerel
           kullandiginin gostergesidir.

Kullanim:
    python3 tools/scan_rule45.py                 # build/ghidra_out/*.c
    python3 tools/scan_rule45.py <dizin>
"""
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def normalize(line):
    """Degisken adlarini soyutla ki ayni sekilli bloklar esitlensin."""
    s = line.strip()
    s = re.sub(r"\blocal_[0-9a-f]+\b", "L", s)
    s = re.sub(r"\b[iup]?[A-Za-z]*Var\d+\b", "V", s)
    s = re.sub(r"\bDAT_[0-9a-f]+\b", "D", s)
    return s


def scan(path):
    lines = [normalize(l) for l in path.read_text(errors="replace").splitlines()]
    lines = [l for l in lines if l and not l.startswith(("/*", "*", "//"))]

    # 6 satirlik pencerelerin tekrarini say
    W = 6
    windows = Counter()
    for i in range(len(lines) - W):
        windows["\n".join(lines[i:i + W])] += 1
    repeats = sum(c - 1 for c in windows.values() if c > 1)

    text = path.read_text(errors="replace")
    locals_ = len(set(re.findall(r"\blocal_[0-9a-f]+\b", text)))
    chain = len(re.findall(r"\bif\s*\(|\belse if\s*\(", text))
    jumptable = "Could not recover jumptable" in text
    return repeats, locals_, chain, jumptable


def main():
    d = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build/ghidra_out"
    if not d.is_dir():
        sys.exit(f"{d} yok. Once toplu cikarimi calistir (bkz. docs/COMPILER.md).")

    rows = []
    for f in sorted(d.glob("*.c")):
        rep, loc, chain, jt = scan(f)
        rows.append((rep, loc, chain, jt, f.stem))
    rows.sort(reverse=True)

    print(f"{'fonksiyon':<20}{'tekrar':>7}{'yerel':>7}{'dal':>5}  durum")
    print("-" * 56)
    for rep, loc, chain, jt, name in rows:
        durum = "Ghidra YARIM" if jt else ("KURAL 45 ADAYI" if rep >= 3 and loc >= 6 else "-")
        print(f"{name:<20}{rep:>7}{loc:>7}{chain:>5}  {durum}")
    n = sum(1 for r in rows if not r[3] and r[0] >= 3 and r[1] >= 6)
    print(f"\n{n} aday (Ghidra ciktisi tam olanlar arasinda)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
