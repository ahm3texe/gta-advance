#!/usr/bin/env python3
"""src/ altindaki C kaynaklarini derleyip ROM ile karsilastirir ve
data/c_sources.csv dosyasini uretir.

Bu, "assembly'den byte-matching" ile "C'den byte-matching" arasindaki ayrimi
olculebilir yapar. Assembly transkripsiyonu ROM'u uretir ama okunabilir kaynak
uretmez; projenin asil hedefi ikincisidir.

Kullanim:  python3 tools/scan_c_sources.py
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import (  # noqa: E402
    DEFAULT_CC, ROM_BASE, compile_and_link, function_rows, rom_bytes,
)

ROOT = Path(__file__).resolve().parent.parent
SOURCES = ROOT / "src"
OUTPUT = ROOT / "data/c_sources.csv"


def main() -> None:
    rom = rom_bytes()
    rows = function_rows()
    records = []
    failures = []

    for source in sorted(SOURCES.rglob("*.c")):
        try:
            blob, layout, _ = compile_and_link(source, DEFAULT_CC)
        except SystemExit as error:
            # Derlenemeyen kaynagi ATLAMAK, o dosyanin fonksiyonlarini
            # c_sources.csv'den sessizce silip komutu yine de basarili
            # gostermek demekti: bozuk bir C dosyasi varken `make check`
            # yesil kaliyordu. Artik toplayip sonunda basarisiz cikiyoruz
            # ve CSV'nin USTUNE YAZMIYORUZ.
            failures.append((source, error))
            continue
        relative = source.relative_to(ROOT)
        for name, (offset, size) in sorted(layout.items(), key=lambda kv: kv[1][0]):
            address = int(rows[name]["address"], 16)
            mapped_size = int(rows[name]["size"], 0)
            start = address - ROM_BASE
            # Yalnizca derleyicinin urettigi kisa bir prefix'in tutmasi tam
            # fonksiyon eslesmesi degildir. Symbol en az haritadaki govdeyi
            # kapsamiyorsa matching terfisi yasaktir.
            complete = size >= mapped_size
            matched = complete and blob[offset:offset + size] == rom[start:start + size]
            records.append({
                "address": f"0x{address:08X}",
                "name": name,
                "source": str(relative),
                "compiled_size": size,
                "mapped_size": mapped_size,
                "matching": "yes" if matched else "no",
            })

    if failures:
        print(f"\nDURDU: {len(failures)} kaynak derlenemedi; "
              f"{OUTPUT.relative_to(ROOT)} DEGISTIRILMEDI.", file=sys.stderr)
        for source, error in failures:
            print(f"  {source.relative_to(ROOT)}: {error}", file=sys.stderr)
        sys.exit(1)

    records.sort(key=lambda r: int(r["address"], 16))
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "address", "name", "source", "compiled_size", "mapped_size", "matching"
            ],
            lineterminator="\n",
        )
        writer.writeheader()
        writer.writerows(records)

    matched = sum(1 for r in records if r["matching"] == "yes")
    print(f"C kaynagi: {len(records)} fonksiyon, {matched} tanesi byte-matching "
          f"-> {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
