#!/usr/bin/env python3
"""Check that the dashboard is only a view of the canonical CSVs."""

import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def csv_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def main() -> int:
    payload = json.loads(
        (ROOT / "dashboard/app/decomp-data.json").read_text(encoding="utf-8")
    )
    functions = {row["address"].upper(): row for row in payload["functions"]}
    c_sources = csv_rows(ROOT / "data/c_sources.csv")
    problems = []

    for source in c_sources:
        function = functions.get(source["address"].upper())
        if function is None:
            problems.append(f"not in the dashboard: {source['address']} {source['name']}")
            continue
        if function["sourceType"] != "c":
            problems.append(f"C source {source['name']} is shown as {function['sourceType']}")
        if function["sourcePath"] != source["source"]:
            problems.append(f"C yolu uyusmuyor: {source['name']}")
        if function["cMatching"] != (source["matching"] == "yes"):
            problems.append(f"C matching bayragi uyusmuyor: {source['name']}")

    for function in functions.values():
        if function["sourceType"] == "none" and function["sourcePath"]:
            problems.append(f"a record without source has a path: {function['name']}")
        if function["sourceType"] == "asm" and function["status"] != "matching":
            problems.append(f"assembly claimed without a record: {function['name']}")

    summary = payload["summary"]
    if summary["cSourceCount"] != len(c_sources):
        problems.append("the dashboard C source count does not agree with c_sources.csv")
    if summary["cMatchingCount"] != sum(row["matching"] == "yes" for row in c_sources):
        problems.append("dashboard C matching sayisi c_sources.csv ile uyusmuyor")
    if payload["workQueue"] != csv_rows(ROOT / "data/work_queue.csv"):
        problems.append("dashboard is kuyrugu work_queue.csv ile uyusmuyor")

    if problems:
        print("GENERATED VIEW INVALID:", file=sys.stderr)
        for problem in problems[:30]:
            print(f"  - {problem}", file=sys.stderr)
        return 1
    print(f"generated views: CLEAN ({len(functions)} functions, {len(c_sources)} C sources)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
