#!/usr/bin/env python3
import csv
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

    fieldnames = ["address", "name", "size", "status", "module", "notes"]
    with TRACKED.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(sorted(merged, key=lambda row: int(row["address"], 16)))

    print(f"Synchronized {len(merged)} functions into {TRACKED} "
          f"({added} added from overrides beyond the Ghidra map)")


if __name__ == "__main__":
    main()
