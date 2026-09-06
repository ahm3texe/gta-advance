#!/usr/bin/env python3
"""Kardes fonksiyon bandinin sabit hedefini ve sonuclarini dogrula."""

import csv
import io
import subprocess
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE_REVISION = "9cbaf27"
MIN_SIZE = 120
MAX_SIZE = 560
MAX_GAP = 200
EXCLUDED_MODULES = {"arm", "libc", "sdk"}


def load_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def baseline_functions():
    result = subprocess.run(
        ["git", "show", f"{BASE_REVISION}:data/functions.csv"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    if result.returncode:
        sys.exit(f"kardes bandi baseline'i okunamadi: {BASE_REVISION}")
    return list(csv.DictReader(io.StringIO(result.stdout)))


def select_band(rows):
    known = []
    for row in rows:
        try:
            known.append((int(row["address"], 16), int(row["size"]), row))
        except (TypeError, ValueError):
            continue
    matching = [(address, size) for address, size, row in known
                if row["status"] == "matching"]
    selected = []
    for address, size, row in known:
        if row["status"] == "matching" or not MIN_SIZE <= size <= MAX_SIZE:
            continue
        if row["module"].lower() in EXCLUDED_MODULES:
            continue
        gap = min(max(0, address - (other + other_size), other - (address + size))
                  for other, other_size in matching)
        if gap <= MAX_GAP:
            selected.append((row["address"].upper().replace("X", "x"), size))
    return selected


def main():
    # Hedef kumesi baseline'da donduruldu: adres + boyut kimliktir. Ad kolonu
    # kimlik degil; fonksiyon tanimlandikca FUN_ yer tutucusundan gercek ada
    # gecer, bu yuzden karsilastirmaya girmez.
    expected = select_band(baseline_functions())
    manifest = load_csv(ROOT / "data/sibling_band.csv")
    actual = [(row["address"], int(row["size"])) for row in manifest]
    errors = []
    if actual != expected:
        errors.append("data/sibling_band.csv hedefleri baseline secimiyle uyusmuyor")

    current = {row["address"].lower(): row
               for row in load_csv(ROOT / "data/functions.csv")}
    for row in manifest:
        address = row["address"].lower()
        outcome = row["outcome"]
        function = current.get(address)
        if function is None:
            errors.append(f"{row['address']}: data/functions.csv kaydi yok")
            continue
        if not row["evidence"].strip():
            errors.append(f"{row['address']}: kanit bos")
        if outcome == "matching":
            if function["status"] != "matching":
                errors.append(f"{row['address']}: manifest matching ama fonksiyon matching degil")
            if not row["source"]:
                errors.append(f"{row['address']}: matching kaynak yolu bos")
        elif outcome == "parked":
            if function["status"] == "matching":
                errors.append(f"{row['address']}: artik matching; manifest guncellenmeli")
            if not row["source"] and not row["evidence"].startswith("ROM statik triyaj:"):
                errors.append(f"{row['address']}: kaynaksiz park icin ROM triyaj kaniti yok")
        else:
            errors.append(f"{row['address']}: gecersiz sonuc {outcome!r}")
        if row["source"] and not (ROOT / row["source"]).is_file():
            errors.append(f"{row['address']}: kaynak bulunamadi: {row['source']}")

    if errors:
        print("KARDES BANDI: HATA")
        for error in errors:
            print(f"  - {error}")
        raise SystemExit(1)

    counts = Counter(row["outcome"] for row in manifest)
    matching_bytes = sum(int(row["size"]) for row in manifest
                         if row["outcome"] == "matching")
    total_bytes = sum(int(row["size"]) for row in manifest)
    print(
        f"KARDES BANDI: TEMIZ — {len(manifest)} hedef / {total_bytes} bayt; "
        f"{counts['matching']} matching ({matching_bytes} bayt), "
        f"{counts['parked']} kanitli park"
    )


if __name__ == "__main__":
    main()
