#!/usr/bin/env python3
"""Ghidra dokumunu function_overrides.csv ile birlestirip functions.csv uretir.

DIKKAT — bu arac ARTIK BIRINCIL KAYNAK DEGIL. Fonksiyon haritasinin
gercegi data/functions.csv'nin KENDISIDIR; onu audit_boundaries.py,
discover_functions.py ve add_c_region.py yeriden
guncelliyor. Bu arac ise haritayi bayat Ghidra dokumunden (2 Eylul)
yeniden kuruyor.

Bu oturumda arac kazara calistirildi ve 479 kaydi sildi: kesfedilen
fonksiyonlar, elle verilen 148 ad ve 133 `matching` durumu kayboldu;
make c-match/rom/check zinciri kirildi. Bu yuzden artik KAYIP KAPISI var:
uretilecek harita mevcut haritadan kayit, ad veya `matching` durumu
kaybediyorsa arac calismayi REDDEDER. Gercekten yeniden kurmak
istiyorsan --force ver.

Kullanim:
    python3 tools/sync_function_map.py            # guvenli, kayip varsa durur
    python3 tools/sync_function_map.py --force    # kaybi kabul et
"""
import csv
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TRACKED = ROOT / "data/functions.csv"
GHIDRA = ROOT / "analysis/function_map_ghidra.csv"
OVERRIDES = ROOT / "data/function_overrides.csv"


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def main() -> None:
    overrides = {row["address"].lower(): row for row in read_rows(OVERRIDES)}
    merged: list[dict[str, str]] = []

    for detected in read_rows(GHIDRA):
        numeric_address = int(detected["address"], 16)
        address = f"0x{numeric_address:08X}"
        override = overrides.get(address.lower())
        row = {
            "address": address,
            "name": override["name"] if override else detected["name"],
            # Override boyutu Ghidra'nin gecici sinirini ezer.
            "size": (override or {}).get("size") or detected["size"],
            "status": override["status"] if override else "candidate",
            "module": override["module"] if override else "unknown",
            "notes": override["notes"] if override else
                "Ghidra auto-analysis; boundary and ARM/Thumb mode are provisional",
        }
        merged.append(row)
        overrides.pop(address.lower(), None)

    # Ghidra'nin kacirdigi ama baska yolla dogrulanan fonksiyonlar
    # (ornegin libc taramasi) yalnizca override dosyasinda bulunur.
    added = 0
    for override in overrides.values():
        if not override.get("size", "").strip():
            print(f"UYARI: {override['address']} Ghidra haritasinda yok ve "
                  f"override'da size verilmemis; atlaniyor.")
            continue
        merged.append({
            "address": f"0x{int(override['address'], 16):08X}",
            "name": override["name"],
            "size": override["size"],
            "status": override["status"],
            "module": override["module"],
            "notes": override["notes"],
        })
        added += 1

    # --- KAYIP KAPISI ---------------------------------------------------
    # Uretilecek harita mevcut haritayla karsilastirilir; kayit, insan
    # verilmis ad veya `matching` durumu kaybi varsa yazma yapilmaz.
    if TRACKED.exists() and "--force" not in sys.argv:
        current = {r["address"].upper(): r for r in read_rows(TRACKED)}
        produced = {r["address"].upper(): r for r in merged}
        lost_rows = [a for a in current if a not in produced]
        lost_names = [a for a, r in current.items()
                      if a in produced
                      and not r["name"].lower().startswith("fun_")
                      and produced[a]["name"] != r["name"]]
        lost_match = [a for a, r in current.items()
                      if r["status"] == "matching"
                      and produced.get(a, {}).get("status") != "matching"]
        if lost_rows or lost_names or lost_match:
            print("DURDU: bu calistirma fonksiyon haritasindan veri kaybederdi.")
            print(f"  {len(lost_rows):4d} kayit silinirdi "
                  f"(mevcut {len(current)} -> uretilen {len(produced)})")
            print(f"  {len(lost_names):4d} insan-verilmis ad FUN_xxxx'e donerdi")
            print(f"  {len(lost_match):4d} `matching` durumu dusurulurdu")
            for address in (lost_names or lost_rows or lost_match)[:5]:
                name = current[address]["name"]
                print(f"    ornek {address} {name}")
            print("\nBirincil kaynak data/functions.csv'dir; bu arac onu bayat "
                  "Ghidra dokumunden yeniden kurar.\nGercekten istiyorsan: "
                  "--force (once `git add data/functions.csv` yapmani oneririm).")
            sys.exit(1)

    fieldnames = ["address", "name", "size", "status", "module", "notes"]
    with TRACKED.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(sorted(merged, key=lambda row: int(row["address"], 16)))

    print(f"Synchronized {len(merged)} functions into {TRACKED} "
          f"({added} added from overrides beyond the Ghidra map)")


if __name__ == "__main__":
    main()
