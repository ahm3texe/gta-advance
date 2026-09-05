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
                  cozuluyor mu; ayni gRam sembolu celiskili C extern turleri
                  tasiyor mu  <-- yeniden adlandirma/tur kirilmasini yakalar
  bicim           CSV'lerde KARISIK satir sonu

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
    # Tek basina LF veya CRLF kabul edilir; ikisinin karisimi edilmez.
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

    # Elle reddedilen sahte girişler tekrar fonksiyon haritasına giremez.
    non_function_path = ROOT / "data/non_function_entries.csv"
    if non_function_path.exists():
        rejected: set[int] = set()
        for row in read(non_function_path):
            address = int(row["address"], 16)
            if address in rejected:
                bad("non-functions", f"yinelenen adres {row['address']}")
            rejected.add(address)
            if address in by_address:
                bad("non-functions", f"{row['address']} hem reddedilmiş giriş hem "
                    f"fonksiyon ({by_address[address]['name']})")

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
        compiled_size = int(row["compiled_size"], 0)
        mapped_size = int(row["mapped_size"], 0)
        function_size = int(target["size"], 0)
        if mapped_size != function_size:
            bad("c_sources", f"{row['name']} mapped_size={mapped_size}, "
                             f"functions size={function_size}; c-status bayat")
        if row["matching"] == "yes" and compiled_size < mapped_size:
            bad("c_sources", f"{row['name']} kisa C prefix'i matching sayilmis: "
                             f"{compiled_size} < {mapped_size}")

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

    # --- ARM inceleme tablosu: bütün overlay aralığını boşluksuz kaplar ---
    arm_review_path = ROOT / "data/arm_boundary_review.csv"
    if arm_review_path.exists():
        arm_ranges = []
        arm_seen: set[int] = set()
        for row in read(arm_review_path):
            address = int(row["address"], 16)
            size = int(row["corrected_size"])
            if address in arm_seen:
                bad("arm-review", f"yinelenen adres {row['address']}")
            arm_seen.add(address)
            target = by_address.get(address)
            if target is None:
                bad("arm-review", f"{row['address']} functions.csv'de yok")
            else:
                if int(target["size"]) != size:
                    bad("arm-review", f"{row['address']} boyut uyusmuyor: "
                        f"review {size}, functions {target['size']}")
                if target["module"] != "arm" or target["status"] in {
                    "candidate", "discovered"
                }:
                    bad("arm-review", f"{row['address']} incelenmis ARM kaydi "
                        f"olarak isaretlenmemis")
            arm_ranges.append((address, address + size))
        arm_ranges.sort()
        if arm_ranges:
            if arm_ranges[0][0] != 0x08067E04 or arm_ranges[-1][1] != 0x0806B84C:
                bad("arm-review", "overlay uçları 0x08067E04-0x0806B84C değil")
            for (_, end), (start, _) in zip(arm_ranges, arm_ranges[1:]):
                if end != start:
                    bad("arm-review", f"0x{end:08X}-0x{start:08X} arasında "
                        "boşluk veya örtüşme var")

    # --- kaynak: her extern sembolu cozuluyor mu (RENAME KIRILMASI) ---
    symbols = set(names) | {r["name"] for r in ram}
    extern = re.compile(r"^\s*extern\s+.*?\b(\w+)\s*(?:\[|\(|;|=)", re.M)
    for source in sorted(ROOT.glob("src/**/*.c")):
        text = source.read_text(encoding="utf-8")
        for name in set(extern.findall(text)):
            # `__thumb` soneki bir SEMBOL DEGIL, cozumleme talimati: aynı
            # adresin Thumb biti kurulu hali demek (tools/agbcc_build.py).
            # Aranirken sonek atilir, yoksa her saklanan fonksiyon isaretcisi
            # sahte "yeniden adlandirma kirilmasi" olarak bildirilir.
            if name.endswith("__thumb"):
                name = name[: -len("__thumb")]
            if name not in symbols:
                bad("kaynak", f"{source.relative_to(ROOT)}: extern '{name}' "
                              f"ne functions.csv ne ram_map.csv'de — "
                              f"yeniden adlandirma kirilmasi olabilir")

    # --- kaynak/header: ayni fiziksel RAM sembolune tek extern turu ---
    ram_extern = re.compile(
        r"^\s*extern\s+(.+?)\s+(\*?)(gRam[0-9A-Fa-f]+)"
        r"(\s*\[[^;]*\])?\s*;",
        re.M,
    )
    ram_types: dict[str, dict[str, list[str]]] = {}
    declaration_files = sorted(ROOT.glob("src/**/*.c")) + sorted(
        ROOT.glob("include/**/*.h")
    )
    for source in declaration_files:
        text = source.read_text(encoding="utf-8")
        for match in ram_extern.finditer(text):
            type_text = " ".join(
                (match.group(1) + match.group(2) + (match.group(4) or "")).split()
            )
            ram_types.setdefault(match.group(3), {}).setdefault(type_text, []).append(
                str(source.relative_to(ROOT))
            )
    for symbol, declarations in sorted(ram_types.items()):
        if len(declarations) > 1:
            detail = "; ".join(
                f"{kind} ({', '.join(paths)})" for kind, paths in declarations.items()
            )
            bad("ram-extern", f"{symbol} celiskili extern turlerinde: {detail}")

    # Ayni SEMBOLU, ayni struct ADIYLA ama FARKLI GOVDEYLE gormek sessiz bir
    # tehlike: extern turu ayni yazildigi icin yukaridaki kontrol yakalamiyor.
    # `Anchor` dort dosyada ayni adla iki farkli govdeyle duruyordu (biri
    # tamamen acilmis, otekiler dolgulu). Yerlesimler uyumlu oldugu icin hata
    # vermiyordu ama birini degistirmek otekini sessizce yanlis yapardi.
    #
    # Kontrol BILEREK dar tutuldu: farkli sembolleri tarif eden ayni adli
    # struct'lar bu projede NORMAL (her ceviri birimi kendi yerel gorunumunu
    # kurar, bkz. include/ram_symbols.h). Yalnizca AYNI sembol uzerinde
    # celisen govdeler bildirilir.
    struct_def = re.compile(
        r"typedef\s+struct\s*(?:\w+)?\s*\{(.*?)\}\s*(\w+)\s*;", re.S
    )
    bodies_by_file: dict[str, dict[str, str]] = {}
    for source in declaration_files:
        text = source.read_text(encoding="utf-8")
        for match in struct_def.finditer(text):
            body = " ".join(
                re.sub(r"/\*.*?\*/", "", match.group(1), flags=re.S).split()
            )
            bodies_by_file.setdefault(str(source.relative_to(ROOT)), {})[
                match.group(2)
            ] = body

    for symbol, declarations in sorted(ram_types.items()):
        for type_text, paths in declarations.items():
            name = type_text.replace("*", "").strip().split()[-1]
            seen: dict[str, list[str]] = {}
            for path in paths:
                body = bodies_by_file.get(path, {}).get(name)
                if body is not None:
                    seen.setdefault(body, []).append(path)
            if len(seen) > 1:
                detail = "; ".join(f"({', '.join(v)})" for v in seen.values())
                bad(
                    "struct-govde",
                    f"{symbol} icin {name} farkli govdelerle tanimli: {detail}",
                )
    # --- Eslesen fonksiyonlarin adi ve notu -------------------------------
    #
    # Bu iki denetim, gercek bir birikme yasandigi icin eklendi (2026-09-06):
    # eslesme akisinda `status` guncelleniyor ama Ghidra'nin YER TUTUCU adina
    # ve notuna donulmuyordu.  365 eslesen fonksiyonun 88'i hala `FUN_` adi
    # tasiyordu ve 60'inin notu "boundary and ARM/Thumb mode are provisional"
    # diyordu -- byte-matching tam olarak bunun tersini kanitlarken.  Mevcut
    # denetimler adlarin dosyalar arasinda TUTARLI olmasina bakiyordu, yer
    # tutucu olup olmadigina degil; bu yuzden hicbir kapi calmadi.
    STALE_NOTE = "boundary and ARM/Thumb mode are provisional"
    placeholder = []
    for row in functions:
        if row.get("status") != "matching":
            continue
        if STALE_NOTE in (row.get("notes") or ""):
            bad(
                "bayat-not",
                f"{row['name']} byte-matching ama notu hala sinirin/kipin "
                f"'gecici' oldugunu soyluyor",
            )
        if row["name"].startswith("FUN_"):
            placeholder.append(row["name"])

    if problems:
        print(f"TUTARSIZLIK: {len(problems)} sorun\n")
        for problem in problems:
            print(f"  {problem}")
        sys.exit(1)
    print(f"tutarlilik: TEMIZ  ({len(functions)} fonksiyon, {len(ram)} RAM sembolu, "
          f"{len(regions)} bolge)")
    # Hata degil, GORUNURLUK: bos saplamalar ve kor iletme sarmalayicilari
    # bilerek adlandirilmiyor (ne yaptiklari bilinmiyor, ad uydurmak olurdu).
    # Sayinin her kosuda yazilmasi, adlandirilabilir olanlarin sessizce
    # birikmesini engelliyor.
    if placeholder:
        print(f"  not: {len(placeholder)} eslesen fonksiyon hala yer tutucu "
              f"`FUN_` adi tasiyor (bos saplama / kor sarmalayici bekleniyor)")


if __name__ == "__main__":
    main()
