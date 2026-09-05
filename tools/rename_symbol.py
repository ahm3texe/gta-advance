#!/usr/bin/env python3
"""Bir sembolu KAYITLARDA ve TUM KAYNAKLARDA birlikte yeniden adlandirir.

Neden var: adlandirmayi elle yapmak bu projede tekrar tekrar externleri
kirdi (bir oturumda dort kez).  functions.csv'de ad degisince baska
dosyalardaki `extern ... FUN_xxxx` bildirimleri cozulemez hale geliyor ve
`make c-match` patliyor.  Bu arac ikisini ATOMIK yapar.

Kullanim:
    python3 tools/rename_symbol.py FUN_08012c0c AllocNode
    python3 tools/rename_symbol.py --dry-run gTableIndex gLanguage

BSD sed NOTU: `sed -i 's/\\bX\\b/Y/'` macOS'ta sessizce hicbir sey yapmaz.
Bu yuzden degistirme Python re.sub ile yapiliyor.
"""
import argparse
import csv
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
RECORDS = [ROOT / "data" / "functions.csv", ROOT / "data" / "ram_map.csv"]
SOURCE_DIRS = [ROOT / "src", ROOT / "include", ROOT / "tools"]
SOURCE_EXT = {".c", ".h"}

IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def rename_records(old, new, dry):
    """Kayit dosyalarinda adi degistir.  (degisen_satir, catisma) dondurur."""
    changed, clash = 0, []
    for path in RECORDS:
        if not path.exists():
            continue
        rows = list(csv.DictReader(path.open()))
        if not rows:
            continue
        fields = list(rows[0].keys())
        if any(r.get("name") == new for r in rows):
            clash.append(f"{path.name}: '{new}' adi ZATEN VAR")
        hit = 0
        for r in rows:
            if r.get("name") == old:
                r["name"] = new
                hit += 1
        if hit and not dry:
            with path.open("w", newline="", encoding="utf-8") as fh:
                w = csv.DictWriter(fh, fieldnames=fields)
                w.writeheader()
                w.writerows(rows)
        changed += hit
        if hit:
            print(f"  {path.relative_to(ROOT)}: {hit} kayit")
    return changed, clash


def rename_sources(old, new, dry):
    pat = re.compile(rf"\b{re.escape(old)}\b")
    total, files = 0, 0
    for base in SOURCE_DIRS:
        if not base.exists():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in SOURCE_EXT or not path.is_file():
                continue
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
            n = len(pat.findall(text))
            if not n:
                continue
            if not dry:
                path.write_text(pat.sub(new, text),
                                encoding="utf-8", errors="surrogateescape")
            print(f"  {path.relative_to(ROOT)}: {n} atif")
            total += n
            files += 1
    return total, files


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("old")
    ap.add_argument("new")
    ap.add_argument("--dry-run", action="store_true",
                    help="hicbir sey yazma, ne olacagini goster")
    ap.add_argument("--force", action="store_true",
                    help="hedef ad zaten varsa yine de devam et")
    a = ap.parse_args()

    for name in (a.old, a.new):
        if not IDENT.match(name):
            print(f"HATA: '{name}' gecerli bir C tanimlayicisi degil")
            return 2
    if a.old == a.new:
        print("HATA: eski ve yeni ad ayni")
        return 2

    tag = "[deneme] " if a.dry_run else ""
    print(f"{tag}{a.old} -> {a.new}")

    print("kayitlar:")
    rec, clash = rename_records(a.old, a.new, a.dry_run)
    if clash and not a.force:
        for c in clash:
            print(f"  HATA: {c}")
        print("  (bilerek birlestiriyorsan --force)")
        return 1

    print("kaynaklar:")
    src, files = rename_sources(a.old, a.new, a.dry_run)

    if rec == 0 and src == 0:
        print(f"UYARI: '{a.old}' hicbir yerde bulunamadi")
        return 1

    print(f"\ntoplam: {rec} kayit, {src} atif ({files} dosya)")
    if a.dry_run:
        print("hicbir sey yazilmadi (--dry-run)")
    else:
        print("simdi calistir: python3 tools/check_consistency.py")
    return 0


if __name__ == "__main__":
    sys.exit(main())
