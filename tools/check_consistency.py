#!/usr/bin/env python3
"""Veri dosyalarinin kendi icinde, birbiriyle ve kaynakla tutarliligini denetler.

Bu arac bir denetimden sonra yazildi: yeniden adlandirma bu projede YEDI kez
baska bir dosyayi kirdi ve her seferinde ancak `make rom` zincirinin sonunda
fark edildi. Ayrica functions.csv'de yinelenen kayit, ust uste binen aralik
ve Thumb'da imkansiz TEK sayili boyut birikmisti; hicbir denetim yakalamiyordu.

Denetlenenler:
  functions.csv   yinelenen adres, ust uste binen aralik, tek sayili boyut,
                  gecersiz status, tek adrese cok isim
  c_sources.csv   adres functions.csv'de var mi, ad tutuyor mu, dosya diskte
                  mi, matching bayragi status ile celisiyor mu
  matching_regions.csv  ust uste binen bolge, hizasiz sinir
  ram_map.csv     yinelenen adres, ayni adrese cok isim, EWRAM/IWRAM tasmasi
  src/**/*.c      her `extern` sembolu functions.csv veya ram_map.csv'de
                  cozuluyor mu  <-- yeniden adlandirma kirilmasini yakalar
  bicim           CSV'lerde KARISIK satir sonu. (Yalnizca CRLF normaldir:
                  csv.DictWriter varsayilan olarak CRLF yazar ve depo baştan
                  beri oyle. Karisik olmasi ise arac disi bir seyin dosyaya
                  dokundugunu gosterir.)

Cikis kodu: sorun varsa 1, temizse 0. `make check` bunu cagirir.
"""
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VALID_STATUS = {"matching", "documented", "decompiled", "discovered", "candidate"}
RAM_REGIONS = [(0x02000000, 0x02040000, "EWRAM"), (0x03000000, 0x03008000, "IWRAM")]


def read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def main() -> None:
    problems: list[str] = []

    def bad(section: str, message: str) -> None:
        problems.append(f"[{section}] {message}")

    # --- bicim: KARISIK satir sonu (arac disi bir seyin yazdiginin isareti).
    # Tek basina CRLF normaldir; csv.DictWriter varsayilani odur.
    for name in ("functions.csv", "c_sources.csv", "matching_regions.csv",
                 "ram_map.csv", "function_overrides.csv"):
        path = ROOT / "data" / name
        if not path.exists():
            continue
        data = path.read_bytes()
        crlf = data.count(b"\r\n")
        bare_lf = data.count(b"\n") - crlf
        if crlf and bare_lf:
            bad("bicim", f"{name} KARISIK satir sonu ({crlf} CRLF, "
                         f"{bare_lf} yalin LF) — arac disi bir sey yazmis")

    # --- functions.csv ---
    functions = read(ROOT / "data/functions.csv")
    by_address: dict[int, dict[str, str]] = {}
    for row in functions:
        address = int(row["address"], 16)
        if address in by_address:
            bad("functions", f"yinelenen adres {row['address']} "
                             f"({by_address[address]['name']} / {row['name']})")
        by_address[address] = row
        if row["status"] not in VALID_STATUS:
            bad("functions", f"{row['address']} gecersiz status '{row['status']}'")
        if row["size"].strip():
            size = int(row["size"])
            if size % 2:
                bad("functions", f"{row['address']} {row['name']} boyut {size} "
                                 f"TEK — Thumb'da imkansiz")

    sized = sorted((int(r["address"], 16), int(r["size"] or 0), r["name"])
                   for r in functions if r["size"].strip())
    for (a, s, n), (b, _, n2) in zip(sized, sized[1:]):
        if a + s > b:
            bad("functions", f"0x{a:08X} ({n}, {s}B) 0x{b:08X} ({n2}) icine "
                             f"{a + s - b} bayt tasiyor")

    names: dict[str, str] = {}
    for row in functions:
        if row["name"] in names and not row["name"].lower().startswith("fun_"):
            bad("functions", f"'{row['name']}' iki adreste: "
                             f"{names[row['name']]} ve {row['address']}")
        names[row["name"]] = row["address"]

    # --- c_sources.csv <-> functions.csv <-> disk ---
    for row in read(ROOT / "data/c_sources.csv"):
        address = int(row["address"], 16)
        target = by_address.get(address)
        if target is None:
            bad("c_sources", f"{row['address']} ({row['name']}) functions.csv'de yok")
            continue
        if target["name"] != row["name"]:
            bad("c_sources", f"{row['address']} ad uyusmuyor: c_sources "
                             f"'{row['name']}' vs functions '{target['name']}'")
        if not (ROOT / row["source"]).exists():
            bad("c_sources", f"{row['source']} diskte yok ({row['name']})")
        if row["matching"] == "yes" and target["status"] != "matching":
            bad("c_sources", f"{row['name']} matching=yes ama functions'ta "
                             f"'{target['status']}'")
        if row["matching"] == "no" and target["status"] == "matching":
            bad("c_sources", f"{row['name']} matching=no ama functions'ta 'matching'")

    # --- matching_regions.csv ---
    regions = sorted((int(r["start"], 16), int(r["end"], 16), r["binary"])
                     for r in read(ROOT / "data/matching_regions.csv"))
    for (s1, e1, b1), (s2, _, b2) in zip(regions, regions[1:]):
        if e1 > s2:
            bad("regions", f"{b1} ve {b2} ust uste biniyor "
                           f"(0x{s1:08X}-0x{e1:08X} vs 0x{s2:08X})")

    # --- ram_map.csv ---
    ram = read(ROOT / "data/ram_map.csv")
    seen_ram: dict[int, str] = {}
    for row in ram:
        address = int(row["address"], 16)
        if address in seen_ram:
            bad("ram_map", f"{row['address']} iki isimde: "
                           f"{seen_ram[address]} ve {row['name']}")
        seen_ram[address] = row["name"]
        size = int(row["size"] or 0)
        for lo, hi, label in RAM_REGIONS:
            if lo <= address < hi and address + size > hi:
                bad("ram_map", f"{row['name']} {label} sonunu "
                               f"{address + size - hi} bayt asiyor")

    # --- kaynak: her extern sembolu cozuluyor mu (RENAME KIRILMASI) ---
    symbols = set(names) | {r["name"] for r in ram}
    extern = re.compile(r"^\s*extern\s+.*?\b(\w+)\s*(?:\[|\(|;|=)", re.M)
    for source in sorted(ROOT.glob("src/**/*.c")):
        text = source.read_text(encoding="utf-8")
        for name in set(extern.findall(text)):
            if name not in symbols:
                bad("kaynak", f"{source.relative_to(ROOT)}: extern '{name}' "
                              f"ne functions.csv ne ram_map.csv'de — "
                              f"yeniden adlandirma kirilmasi olabilir")

    if problems:
        print(f"TUTARSIZLIK: {len(problems)} sorun\n")
        for problem in problems:
            print(f"  {problem}")
        sys.exit(1)
    print(f"tutarlilik: TEMIZ  ({len(functions)} fonksiyon, {len(ram)} RAM sembolu, "
          f"{len(regions)} bolge)")


if __name__ == "__main__":
    main()
