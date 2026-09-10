#!/usr/bin/env python3
"""Generate the objdiff-schema progress report consumed by decomp.dev.

The report is derived entirely from the project's own recorded ground truth,
`data/functions.csv` and `data/c_sources.csv`, so it needs no ROM and no build.
Both files are regenerated and re-verified against the ROM by `make check`, so
a report produced here can never claim more than the last verified state.

Every function in the map becomes part of exactly one unit, and the unit sizes
sum to the full ROM code size. Reporting only the decompiled part would inflate
the percentage, which is the one mistake this file exists to avoid.

THE MAP IS NOT THE WHOLE CODE REGION. Stretches of the image lie between one
map entry's end and the next one's start, and some of them hold instructions no
entry covers -- tools/find_map_gaps.py measures how many. Left out, they are
missing from the DENOMINATOR and every percentage comes out high; at the time
this was written, 14.45% instead of 14.20%. They are therefore included, as
their own unit, with nothing in them matched. Each is a REGION and not a
function: the names say so, and the unit's name does too, because the function
count they contribute to is a count of regions and not of the functions inside
them. That direction is the safe one -- it understates progress rather than
overstating it -- and it corrects itself as the map is repaired.

Units are formed as follows:
  * one unit per C source file, for the functions that have one;
  * one unit per module for the rest, except the unnamed bulk of the ROM,
    which is split into fixed address bands so the treemap stays readable.

Usage:
    python3 tools/gen_report.py                 # write report.json
    python3 tools/gen_report.py -o build/x.json # write elsewhere
    python3 tools/gen_report.py --check         # print the summary, write nothing

Schema: https://github.com/encounter/objdiff/blob/main/objdiff-core/protos/report.proto
"""

import argparse
import csv
import json
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import find_map_gaps  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS_CSV = ROOT / "data" / "functions.csv"
C_SOURCES_CSV = ROOT / "data" / "c_sources.csv"

# objdiff rejects any report whose version does not migrate to this.
REPORT_VERSION = 2

ROM_BASE = 0x08000000

# The unnamed bulk of the ROM is one module in functions.csv. Reporting it as a
# single unit would produce one block covering most of the treemap, so it is cut
# into bands of this size instead. Bands are a presentation device only; they do
# not claim the enclosed functions belong together.
BAND_SIZE = 0x10000

MATCHING = "matching"

# The unit that holds the code no map entry covers. Named so that a reader of
# the treemap can see what it is without opening this file.
UNMAPPED_UNIT = "rom/unmapped (no entry in the function map)"


def read_rows(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        sys.exit(f"{path.relative_to(ROOT)} is missing. First run: make check")
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def percent(part: int, whole: int) -> float:
    return round(100.0 * part / whole, 4) if whole else 0.0


def measures(functions: list[dict], matched: list[dict], complete: bool) -> dict:
    """Build one Measures message.

    `complete` means the unit is fully linked. objdiff keeps "matched" and
    "complete" apart: a unit can be fully matched byte for byte and still not be
    linked into a whole binary. This project links each function on its own, so
    a fully matched unit is a complete one.
    """
    total_code = sum(f["size"] for f in functions)
    matched_code = sum(f["size"] for f in matched)
    return {
        "fuzzy_match_percent": percent(matched_code, total_code),
        "total_code": total_code,
        "matched_code": matched_code,
        "matched_code_percent": percent(matched_code, total_code),
        "total_data": 0,
        "matched_data": 0,
        "matched_data_percent": 0.0,
        "total_functions": len(functions),
        "matched_functions": len(matched),
        "matched_functions_percent": percent(len(matched), len(functions)),
        "complete_code": matched_code if complete else 0,
        "complete_code_percent": percent(matched_code, total_code) if complete else 0.0,
        "complete_data": 0,
        "complete_data_percent": 0.0,
        "total_units": 1,
        "complete_units": 1 if complete else 0,
    }


def make_unit(name: str, functions: list[dict], source_path: str | None) -> dict:
    functions = sorted(functions, key=lambda f: f["address"])
    matched = [f for f in functions if f["matching"]]
    complete = bool(functions) and len(matched) == len(functions)
    unit = {
        "name": name,
        "measures": measures(functions, matched, complete),
        "sections": [],
        "functions": [
            {
                "name": f["name"],
                "size": f["size"],
                # This project records a whole-function byte comparison, so a
                # function is either identical to the ROM or it is not. There is
                # no partial score to report.
                "fuzzy_match_percent": 100.0 if f["matching"] else 0.0,
                "metadata": {"virtual_address": f["address"]},
            }
            for f in functions
        ],
        "metadata": {"complete": complete, "source_path": source_path},
    }
    return unit


def load_unmapped() -> list[dict]:
    """Return the code stretches no map entry covers, as pseudo-functions.

    Measured by tools/find_map_gaps.py, which is the authority on what counts:
    it drops the alignment padding and the over-large stretches that are data
    rather than one function's worth of missing code.

    These are REGIONS. Each one may hold several functions, so the count they
    contribute is a count of regions; the names begin with `unmapped_` so that
    is visible wherever they are listed.
    """
    rom = find_map_gaps.rom_bytes()
    out = []
    for start, length, _, _ in find_map_gaps.gaps(find_map_gaps.entries()):
        if length > find_map_gaps.MAX_GAP:
            continue
        body = rom[start - ROM_BASE:start - ROM_BASE + length]
        kind, _ = find_map_gaps.classify(body, start)
        if kind == "padding":
            continue
        out.append({
            "address": start,
            "name": f"unmapped_{start:08X}",
            "size": length,
            "module": "unmapped",
            "matching": False,
            "source": None,
        })
    return out


def load_functions() -> list[dict]:
    """Return every mapped function, annotated with its source file if it has one."""
    source_of = {
        row["address"].upper(): row["source"]
        for row in read_rows(C_SOURCES_CSV)
    }
    functions = []
    for row in read_rows(FUNCTIONS_CSV):
        address = row["address"].upper()
        functions.append({
            "address": int(address, 16),
            "name": row["name"],
            "size": int(row["size"]),
            "module": row["module"],
            "matching": row["status"] == MATCHING,
            "source": source_of.get(address),
        })
    return functions


def build_units(functions: list[dict]) -> list[dict]:
    by_source: dict[str, list[dict]] = defaultdict(list)
    by_module: dict[str, list[dict]] = defaultdict(list)
    for function in functions:
        if function["source"]:
            by_source[function["source"]].append(function)
        else:
            by_module[function["module"]].append(function)

    units = [
        make_unit(source.removeprefix("src/").removesuffix(".c"), fns, source)
        for source, fns in by_source.items()
    ]

    for module, fns in by_module.items():
        if module == "unmapped":
            units.append(make_unit(UNMAPPED_UNIT, fns, None))
            continue
        if module != "unknown":
            units.append(make_unit(f"{module} (no source yet)", fns, None))
            continue
        bands: dict[int, list[dict]] = defaultdict(list)
        for function in fns:
            bands[function["address"] // BAND_SIZE].append(function)
        for band, band_fns in bands.items():
            start = band * BAND_SIZE
            units.append(make_unit(
                f"rom/0x{start:08X}-0x{start + BAND_SIZE:08X}", band_fns, None))

    units.sort(key=lambda u: u["functions"][0]["metadata"]["virtual_address"])
    return units


def total_measures(units: list[dict]) -> dict:
    keys = ("total_code", "matched_code", "total_functions", "matched_functions",
            "complete_code", "total_units", "complete_units")
    total = {key: 0 for key in keys}
    for unit in units:
        for key in keys:
            total[key] += unit["measures"][key]
    return {
        "fuzzy_match_percent": percent(total["matched_code"], total["total_code"]),
        "total_code": total["total_code"],
        "matched_code": total["matched_code"],
        "matched_code_percent": percent(total["matched_code"], total["total_code"]),
        "total_data": 0,
        "matched_data": 0,
        "matched_data_percent": 0.0,
        "total_functions": total["total_functions"],
        "matched_functions": total["matched_functions"],
        "matched_functions_percent": percent(
            total["matched_functions"], total["total_functions"]),
        "complete_code": total["complete_code"],
        "complete_code_percent": percent(total["complete_code"], total["total_code"]),
        "complete_data": 0,
        "complete_data_percent": 0.0,
        "total_units": total["total_units"],
        "complete_units": total["complete_units"],
    }


def verify(report: dict, functions: list[dict]) -> None:
    """Fail closed rather than publish a figure the project cannot stand behind."""
    m = report["measures"]
    problems = []
    if report["version"] != REPORT_VERSION:
        problems.append(f"report version {report['version']} != {REPORT_VERSION}")
    if m["total_units"] != len(report["units"]):
        problems.append("total_units disagrees with the number of units")
    if m["matched_code"] > m["total_code"]:
        problems.append("matched_code exceeds total_code")

    expected_code = sum(f["size"] for f in functions)
    expected_matched = sum(f["size"] for f in functions if f["matching"])
    if m["total_code"] != expected_code:
        problems.append(
            f"total_code {m['total_code']} != {expected_code} in functions.csv; "
            "a function is missing from the units and the percentage would be inflated")
    if m["matched_code"] != expected_matched:
        problems.append(
            f"matched_code {m['matched_code']} != {expected_matched} in functions.csv")
    if m["total_functions"] != len(functions):
        problems.append(
            f"total_functions {m['total_functions']} != {len(functions)} in functions.csv")

    for unit in report["units"]:
        um = unit["measures"]
        if um["matched_code"] > um["total_code"]:
            problems.append(f"{unit['name']}: matched_code exceeds total_code")
        if um["complete_units"] and um["matched_code"] != um["total_code"]:
            problems.append(f"{unit['name']}: marked complete but not fully matched")

    if problems:
        for problem in problems:
            print(f"ERROR: {problem}", file=sys.stderr)
        sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("-o", "--output", default="report.json",
                        help="where to write the report (default: report.json)")
    parser.add_argument("--check", action="store_true",
                        help="print the summary and verify, but write nothing")
    args = parser.parse_args()

    functions = load_functions() + load_unmapped()
    units = build_units(functions)
    report = {
        "measures": total_measures(units),
        "units": units,
        "version": REPORT_VERSION,
        "categories": [],
    }
    verify(report, functions)

    m = report["measures"]
    summary = (
        f"code: {m['matched_code']}/{m['total_code']} bytes "
        f"({m['matched_code_percent']:.2f}%) | "
        f"functions: {m['matched_functions']}/{m['total_functions']} | "
        f"units: {m['complete_units']}/{m['total_units']} complete"
    )

    if args.check:
        print(f"progress report: CLEAN ({summary})")
        return

    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {out} ({out.stat().st_size} bytes)")
    print(summary)


if __name__ == "__main__":
    main()
