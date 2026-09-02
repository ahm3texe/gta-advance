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
            "size": detected["size"],
            "status": override["status"] if override else "candidate",
            "module": override["module"] if override else "unknown",
            "notes": override["notes"] if override else
                "Ghidra auto-analysis; boundary and ARM/Thumb mode are provisional",
        }
        merged.append(row)

    fieldnames = ["address", "name", "size", "status", "module", "notes"]
    with TRACKED.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(sorted(merged, key=lambda row: int(row["address"], 16)))

    print(f"Synchronized {len(merged)} Ghidra candidates into {TRACKED}")


if __name__ == "__main__":
    main()
