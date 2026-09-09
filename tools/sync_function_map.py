#!/usr/bin/env python3
"""Merge the Ghidra dump with function_overrides.csv to produce functions.csv.

WARNING -- this tool is NO LONGER THE PRIMARY SOURCE. The truth of the function
map is data/functions.csv ITSELF; audit_boundaries.py, discover_functions.py and
add_c_region.py update it in place. This tool instead rebuilds the map from a
stale Ghidra dump (2 September).

In one session this tool was run by accident and deleted 479 records: discovered
functions, 148 hand-given names and 133 `matching` states were lost, and the
make c-match/rom/check chain broke. So there is now a LOSS GATE: if the map it
would produce loses records, names or `matching` states relative to the current
map, the tool REFUSES to run. If you really want to rebuild, pass --force.

Usage:
    python3 tools/sync_function_map.py            # safe, stops on any loss
    python3 tools/sync_function_map.py --force    # accept the loss
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
            print(f"WARNING: {override['address']} is not in the Ghidra map and "
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
    # The map to be produced is compared against the current one; records, human
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
            print("STOPPED: this run would lose data from the function map.")
            print(f"  {len(lost_rows):4d} records would be deleted "
                  f"(mevcut {len(current)} -> uretilen {len(produced)})")
            print(f"  {len(lost_names):4d} insan-verilmis ad FUN_xxxx'e donerdi")
            print(f"  {len(lost_match):4d} `matching` durumu dusurulurdu")
            for address in (lost_names or lost_rows or lost_match)[:5]:
                name = current[address]["name"]
                print(f"    ornek {address} {name}")
            print("\nThe primary source is data/functions.csv; this tool rebuilds it "
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
