#!/usr/bin/env python3
"""Bir C kaynagi uzerinde YAPISAL VARYANTLARI tarayip ROM ile karsilastirir.

Neden var
---------
Yakin iskalarda elle deneme ve decomp-permuter'in ikisi de tikaniyor ama
SISTEMATIK tarama cozüyor.  Olculdu (2026-09-05, EitherInRange 8/44):

  - elle 5040 bildirim sirasi denendi -> hicbiri 8'in altina inmedi
  - decomp-permuter 20000 iterasyon kostu -> skor 205'te plato
  - bu aractaki "karsilastirma sabitini yerele al" taramasi -> 0/44

Permuter RASTGELE yerel mutasyon yapar; bu arac BELIRLI mekanizmalarin
TUM alt kumelerini dener.  Ikisi farkli seyler icin iyi: permuter genis ve
kor, bu arac dar ve tuketici.

KAPSAM SINIRI -- OLCULDU, ABARTMA
---------------------------------
Bu arac YALNIZCA tanidigi mekanizmalari tarar.  Mevcut uc yakin iskada
(2026-09-05) hicbir sey bulamadi:

    ClearTextArea    1/132   -> 17 varyant, sonuc yok
    CleanupAreaTiles 7/88    ->  2 varyant, sonuc yok
    FUN_08045AA4    12/540   ->  2 varyant, sonuc yok

Sebep: ucunun de tikanmasi YUKLEME/SAKLAMA YAZMAC ATAMASI, karsilastirma
kanonikleştirmesi degil.  Bu sinif icin bilinen bir kaynak-duzeyi kaldirac
YOK.  Akla gelen ilk fikir olan BILDIRIM SIRASI taramasi da bos: onceki
model ClearTextArea icin 5040 sıralamayi tuketmis, hicbiri iyilesme
vermemis.  Bilinmeyen mekanizmaya tarama yazilamaz.

Yani bu arac, kural 44 tipi (karsilastirma sabiti) iskalar icin dogru ve
hizli; yazmac dagitimi iskalari icin CARE DEGIL.  Yeni bir mekanizma
kesfedilirse TRANSFORMS sozlugune eklenmeli.

Kullanim
--------
    python3 tools/sweep_variants.py <kaynak.c> <FonksiyonAdi>
    python3 tools/sweep_variants.py <kaynak.c> <Ad> --only hoist_cmp
    python3 tools/sweep_variants.py <kaynak.c> <Ad> --max-sites 8 --keep

Kaynak dosya DEGISTIRILMEZ; en iyi varyant --keep verilirse yazilir.
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

# ---------------------------------------------------------------- yardimcilar

def function_row(name):
    import csv
    for r in csv.DictReader((ROOT / "data/functions.csv").open()):
        if r["name"] == name:
            return int(r["address"], 16), int(r["size"] or 0)
    sys.exit(f"{name} data/functions.csv icinde yok")


def body_span(text, name):
    """Fonksiyon govdesinin (acilis susu dahil) [start, end) araligi."""
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


# ------------------------------------------------------------ donusturuculer
# Her donusturucu: (ad, govde) -> [(site_aciklama, uygula_fn), ...]
# uygula_fn(govde) -> yeni govde.  Uygulamalar ARKADAN ONE dogru yapilir ki
# ofsetler kaymasin.

CMP_LITERAL = re.compile(r"(\b[A-Za-z_]\w*(?:->|\.)?\w*)\s*(<=|>=|<|>|==|!=)\s*(\d+)\b")


def sites_hoist_cmp(body, decl_anchor):
    """Kural 44: karsilastirma sabitini yerel degiskene al.

    agbcc `x < 15`i `x <= 14`e kanonikleştiriyor; sabit degiskende olunca
    kanonikleştirme atlaniyor ve sabit yine immediate olarak yayiliyor.
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
    """Karsilastirma operandlarini takas et (a<b -> b>a).  Yayilma sirasini
    ve dolayisiyla havuz/yazmac atamasini degistirebiliyor."""
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
    """`a = X op Y;` -> ara degiskene bol.  Canli deger sayisini ve
    dolayisiyla dagitimi degistirir."""
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


# ------------------------------------------------------------------- tarama

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
                    help=f"yalnizca bu donusum ({', '.join(TRANSFORMS)})")
    ap.add_argument("--max-sites", type=int, default=10,
                    help="bir donusumde en fazla kac site (2^n kombinasyon)")
    ap.add_argument("--keep", action="store_true",
                    help="en iyi varyanti kaynak dosyaya YAZ")
    a = ap.parse_args()

    src = Path(a.source)
    addr, size = function_row(a.function)
    rom = rom_bytes()
    original = src.read_text()
    span = body_span(original, a.function)
    if not span:
        sys.exit(f"{a.function} govdesi {src} icinde bulunamadi")
    bstart, bend = span
    head, body, tail = original[:bstart], original[bstart:bend], original[bend:]
    decl_anchor = body.index("\n") + 1   # acilis susundan sonraki satir

    base_score, base_size = score(src, a.function, addr, size, rom)
    if base_score is None:
        sys.exit("taban surum derlenmedi")
    print(f"taban: {base_size}/{size} bayt, fark {base_score}")
    if base_score == 0:
        print("zaten eslesiyor"); return 0

    names = a.only or list(TRANSFORMS)
    best = (base_score, None, "taban")
    tried = 0
    work = Path(tempfile.mkdtemp(prefix="sweep_")) / src.name

    for tname in names:
        sites = TRANSFORMS[tname](body, decl_anchor)
        if not sites:
            print(f"  {tname}: site yok"); continue
        if len(sites) > a.max_sites:
            print(f"  {tname}: {len(sites)} site -> ilk {a.max_sites} ile sinirlandi "
                  f"(KAPSAM DUSURULDU)")
            sites = sites[:a.max_sites]
        n = len(sites)
        print(f"  {tname}: {n} site, {2**n} kombinasyon")
        for r in range(1, n + 1):
            for combo in itertools.combinations(range(n), r):
                b = body
                decls = ""
                for k in sorted(combo, reverse=True):   # arkadan one
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
                    print(f"    yeni en iyi: fark {sc} boyut {sz} <- {tname}{list(combo)}")
                if sc == 0:
                    print(f"\n*** ESLESTI *** {tname}{list(combo)}  ({tried} varyant denendi)")
                    if a.keep:
                        src.write_text(best[1]); print(f"    yazildi: {src}")
                    else:
                        out = src.with_suffix(".match.c")
                        out.write_text(best[1]); print(f"    kaydedildi: {out}")
                    return 0

    print(f"\n{tried} varyant denendi; en iyi fark {best[0]} ({best[2]})")
    if best[1] and best[0] < base_score:
        out = src.with_suffix(".best.c")
        out.write_text(best[1]); print(f"en iyi varyant: {out}")
        if a.keep:
            src.write_text(best[1]); print(f"kaynak guncellendi: {src}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
