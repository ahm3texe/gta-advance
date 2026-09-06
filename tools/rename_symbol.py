#!/usr/bin/env python3
"""Bir fonksiyon veya RAM sembolunu TUM depoda tek islemde yeniden adlandirir.

NEDEN VAR
---------
Adlandirma uc kez ayni sekilde kirildi: `data/functions.csv`'deki ad
degistirildi ama eski adi kullanan C kaynaklari guncellenmedi.  Sonuc her
seferinde ayni oldu -- `agbcc_build.py` sembolu cozemedi ve hata ancak
dakikalar sonra `make check` sirasinda, baska islerin ustune yigilmis
halde ciktı.

Bu arac ismi tek yerde degil, ismin gectigi HER yerde degistirir:
veri tablolari, C kaynaklari, basliklar ve belgeler.  Once adresin
gercekten o adi tasidigini dogrular, sonra yeni adin baskasinda kullanilmadigini
kontrol eder; ikisi de tutmazsa hicbir sey yazmaz.

KULLANIM
--------
  python3 tools/rename_symbol.py 0x0803C400 GetOwnerSlot
  python3 tools/rename_symbol.py --dry-run 0x08066A54 ResetLinkHardware
"""

import argparse
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLES = ["data/functions.csv", "data/function_overrides.csv", "data/ram_map.csv"]
TEXT_DIRS = ["src", "include", "docs", "data"]


def load(path):
    with (ROOT / path).open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    return rows


def save(path, rows):
    with (ROOT / path).open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("address", help="0x08066A54 gibi")
    parser.add_argument("new_name")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    address = args.address.upper().replace("X", "x")
    new_name = args.new_name

    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", new_name):
        sys.exit(f"gecersiz ad: {new_name!r}")

    old_name = None
    for table in TABLES:
        for row in load(table):
            if row["address"].upper().replace("X", "x") == address:
                old_name = row["name"]
                break
        if old_name:
            break
    if old_name is None:
        sys.exit(f"{address} hicbir veri tablosunda yok")
    if old_name == new_name:
        sys.exit(f"{address} zaten {new_name}")

    # Yeni ad baska bir adreste kullaniliyorsa dur: sessiz cakisma en kotusu.
    for table in TABLES:
        for row in load(table):
            if (row["name"] == new_name
                    and row["address"].upper().replace("X", "x") != address):
                sys.exit(f"{new_name} zaten {row['address']} tarafindan kullaniliyor")

    pattern = re.compile(rf"\b{re.escape(old_name)}\b")
    touched = []

    for table in TABLES:
        rows = load(table)
        changed = False
        for row in rows:
            for key, value in row.items():
                if value and pattern.search(value):
                    row[key] = pattern.sub(new_name, value)
                    changed = True
        if changed:
            touched.append(table)
            if not args.dry_run:
                save(table, rows)

    for directory in TEXT_DIRS:
        for path in sorted((ROOT / directory).rglob("*")):
            if not path.is_file() or path.suffix not in {".c", ".h", ".s", ".md", ".csv"}:
                continue
            rel = str(path.relative_to(ROOT))
            if rel in TABLES:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            if not pattern.search(text):
                continue
            touched.append(rel)
            if not args.dry_run:
                path.write_text(pattern.sub(new_name, text), encoding="utf-8")

    prefix = "[deneme] " if args.dry_run else ""
    print(f"{prefix}{address}: {old_name} -> {new_name}  ({len(touched)} dosya)")
    for rel in touched:
        print(f"  {rel}")


if __name__ == "__main__":
    main()
