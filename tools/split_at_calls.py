#!/usr/bin/env python3
"""Cagri hedeflerinde birlesmis fonksiyon kayitlarini ayirir.

audit_boundaries.py'nin yurutucusu kosulsuz `b` komutunu fonksiyon ICI
akis sayiyor. Ama GCC `b`'yi KUYRUK CAGRISI icin de kullanir: bir
fonksiyon isini bitirip komsusuna atlar. Bu yuzden Faz 0'da ardisik
fonksiyonlar tek kayda birlestirildi.

Olcut kesin: bir `bl` hedefi asla fonksiyon ORTASI olamaz. Bilinen kodun
icindeki her cagri hedefi bir fonksiyon sinniridir; kayit orada bolunur.

Kullanim:
    python3 tools/split_at_calls.py            # yalnizca rapor
    python3 tools/split_at_calls.py --apply    # functions.csv'yi bol
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000


def call_targets(rom: bytes, rows: list[tuple[int, int]]) -> set[int]:
    targets = set()
    for address, size in rows:
        base = address - ROM_BASE
        for i in range(0, max(0, size - 2), 2):
            hw1 = int.from_bytes(rom[base + i:base + i + 2], "little")
            hw2 = int.from_bytes(rom[base + i + 2:base + i + 4], "little")
            if (hw1 & 0xF800) == 0xF000 and (hw2 & 0xF800) == 0xF800:
                offset = hw1 & 0x7FF
                if offset & 0x400:
                    offset -= 0x800
                targets.add(address + i + 4 + (offset << 12) + ((hw2 & 0x7FF) << 1))
    return targets


def main() -> None:
    apply = "--apply" in sys.argv
    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fields = list(rows[0].keys())

    sized = [(int(r["address"], 16), int(r["size"] or 0)) for r in rows
             if r["size"].strip()]
    targets = call_targets(rom, sized)
    starts = {a for a, _ in sized}

    # Her kayit icin, govdesinin icine dusen cagri hedefleri = bolme noktalari
    splits: dict[int, list[int]] = {}
    for address, size in sized:
        inner = sorted(t for t in targets
                       if address < t < address + size and t not in starts)
        if inner:
            splits[address] = inner

    total_new = sum(len(v) for v in splits.values())
    print(f"{len(splits)} kayit bolunecek, {total_new} yeni fonksiyon sinniri\n")
    print(f"{'kayit':12} {'boyut':>6}  bolme noktalari")
    print("-" * 60)
    for address in sorted(splits)[:15]:
        points = " ".join(f"0x{t:08X}" for t in splits[address][:4])
        size = dict(sized)[address]
        more = "..." if len(splits[address]) > 4 else ""
        print(f"0x{address:08X} {size:>6}  {points}{more}")

    if not apply:
        print("\n(yalnizca rapor; bolmek icin --apply)")
        return

    by_address = {int(r["address"], 16): r for r in rows}
    out = []
    for row in rows:
        address = int(row["address"], 16)
        if address not in splits:
            out.append(row)
            continue
        size = int(row["size"])
        points = splits[address] + [address + size]
        # Ilk parca eski kaydin adini korur, kisalir.
        row["size"] = str(points[0] - address)
        note = row["notes"].strip('"')
        row["notes"] = (note + "; kuyruk cagrisiyla birlesmisti, "
                        "split_at_calls.py ayirdi").lstrip("; ")
        out.append(row)
        for i, start in enumerate(splits[address]):
            out.append({
                "address": f"0x{start:08X}", "name": f"FUN_{start:08x}",
                "size": str(points[i + 1] - start), "status": "discovered",
                "module": row["module"],
                "notes": f"0x{address:08X} kaydinin icinde saklaniyordu; "
                         f"cagri hedefi oldugu icin ayri fonksiyon",
            })
    out.sort(key=lambda r: int(r["address"], 16))
    with FUNCTIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(out)
    print(f"\nfunctions.csv: {len(splits)} kayit bolundu, "
          f"{total_new} yeni fonksiyon.")


if __name__ == "__main__":
    main()
