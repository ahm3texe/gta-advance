#!/usr/bin/env python3
"""Validate the sibling function band's fixed target set and its outcomes."""

import csv
import io
import subprocess
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# The band's target set is fixed as of this commit, so this is a pin into the
# repository's history and anything that rewrites history invalidates it. That
# has happened once: rewriting every commit's author retired 9cbaf27 and this
# became 393b82e, the same tree at the same position.
BASE_REVISION = "393b82e"
MIN_SIZE = 120
MAX_SIZE = 560
MAX_GAP = 200
EXCLUDED_MODULES = {"arm", "libc", "sdk"}
# Machine-read prefix: a park without a source must carry this marker in its
# evidence column of data/sibling_band.csv.
TRIAGE_PREFIX = "ROM static triage:"


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
        sys.exit(
            f"Could not read the sibling band baseline: {BASE_REVISION}\n"
            "  The band's target set is fixed as of that commit, so this needs the\n"
            "  repository's history. In CI a SHALLOW CLONE is the usual cause:\n"
            "  actions/checkout defaults to fetch-depth 1, which omits it.\n"
            f"  Locally: git fetch --unshallow, or verify with git cat-file -e {BASE_REVISION}")
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
    # The target set is frozen in the baseline: address + size is the identity.
    # The name column is not identity; as a function is identified it moves from
    # the FUN_ placeholder to a real name, so it is excluded from the comparison.
    expected = select_band(baseline_functions())
    manifest = load_csv(ROOT / "data/sibling_band.csv")
    actual = [(row["address"], int(row["size"])) for row in manifest]
    errors = []
    if actual != expected:
        errors.append("the targets in data/sibling_band.csv do not match the baseline selection")

    current = {row["address"].lower(): row
               for row in load_csv(ROOT / "data/functions.csv")}
    for row in manifest:
        address = row["address"].lower()
        outcome = row["outcome"]
        function = current.get(address)
        if function is None:
            errors.append(f"{row['address']}: no record in data/functions.csv")
            continue
        if not row["evidence"].strip():
            errors.append(f"{row['address']}: evidence is empty")
        if outcome == "matching":
            if function["status"] != "matching":
                errors.append(f"{row['address']}: the manifest says matching but the function is not")
            if not row["source"]:
                errors.append(f"{row['address']}: the matching source path is empty")
        elif outcome == "parked":
            if function["status"] == "matching":
                errors.append(f"{row['address']}: it is matching now; the manifest must be updated")
            if not row["source"] and not row["evidence"].startswith(TRIAGE_PREFIX):
                errors.append(f"{row['address']}: no ROM triage evidence for a park without source")
        else:
            errors.append(f"{row['address']}: invalid outcome {outcome!r}")
        if row["source"] and not (ROOT / row["source"]).is_file():
            errors.append(f"{row['address']}: source not found: {row['source']}")

    if errors:
        print("SIBLING BAND: ERROR")
        for error in errors:
            print(f"  - {error}")
        raise SystemExit(1)

    counts = Counter(row["outcome"] for row in manifest)
    matching_bytes = sum(int(row["size"]) for row in manifest
                         if row["outcome"] == "matching")
    total_bytes = sum(int(row["size"]) for row in manifest)
    print(
        f"SIBLING BAND: CLEAN - {len(manifest)} targets / {total_bytes} bytes; "
        f"{counts['matching']} matching ({matching_bytes} bytes), "
        f"{counts['parked']} evidenced parks"
    )


if __name__ == "__main__":
    main()
