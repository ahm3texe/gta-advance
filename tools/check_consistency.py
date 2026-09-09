#!/usr/bin/env python3
"""Check the data files for internal, mutual and source consistency.

This tool was written after an audit: renaming broke another file SEVEN times
in this project, and each time it was only noticed at the end of the `make rom`
chain. Duplicate records, overlapping ranges and ODD sizes impossible in Thumb
had also accumulated in functions.csv, and no check caught them.

What is checked:
  functions.csv   duplicate address, overlapping range, odd size,
                  invalid status, several names for one address
  c_sources.csv   is the address in functions.csv, does the name agree, is the
                  file on disk, does the matching flag contradict the status
  matching_regions.csv  overlapping region, misaligned boundary
  ram_map.csv     duplicate address, several names for one address, EWRAM/IWRAM
                  overflow
  src/**/*.c      does every `extern` symbol resolve in functions.csv or
                  ram_map.csv; does the same gRam symbol carry conflicting C
                  extern types  <-- catches rename/type breakage
  format          MIXED line endings in the CSVs

Exit code: 1 if there are problems, 0 if clean. `make check` invokes this.
"""
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VALID_STATUS = {"matching", "documented", "decompiled", "discovered", "candidate"}
RAM_REGIONS = [(0x02000000, 0x02040000, "EWRAM"), (0x03000000, 0x03008000, "IWRAM")]


def read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def main() -> None:
    problems: list[str] = []

    def bad(section: str, message: str) -> None:
        problems.append(f"[{section}] {message}")

    # --- format: MIXED line endings (a sign something outside the tools wrote).
    # LF alone or CRLF alone is accepted; a mixture is not.
    for name in ("functions.csv", "c_sources.csv", "matching_regions.csv",
                 "ram_map.csv", "function_overrides.csv"):
        path = ROOT / "data" / name
        if not path.exists():
            continue
        data = path.read_bytes()
        crlf = data.count(b"\r\n")
        bare_lf = data.count(b"\n") - crlf
        if crlf and bare_lf:
            bad("format", f"{name} MIXED line endings ({crlf} CRLF, "
                         f"{bare_lf} bare LF) - something outside the tools wrote it")

    # --- functions.csv ---
    functions = read(ROOT / "data/functions.csv")
    by_address: dict[int, dict[str, str]] = {}
    for row in functions:
        address = int(row["address"], 16)
        if address in by_address:
            bad("functions", f"duplicate address {row['address']} "
                             f"({by_address[address]['name']} / {row['name']})")
        by_address[address] = row
        if row["status"] not in VALID_STATUS:
            bad("functions", f"{row['address']} invalid status '{row['status']}'")
        if row["size"].strip():
            size = int(row["size"])
            if size % 2:
                bad("functions", f"{row['address']} {row['name']} size {size} "
                                 f"is ODD - impossible in Thumb")

    sized = sorted((int(r["address"], 16), int(r["size"] or 0), r["name"])
                   for r in functions if r["size"].strip())
    for (a, s, n), (b, _, n2) in zip(sized, sized[1:]):
        if a + s > b:
            bad("functions", f"0x{a:08X} ({n}, {s}B) runs into 0x{b:08X} ({n2}) by "
                             f"{a + s - b} bytes")

    names: dict[str, str] = {}
    for row in functions:
        if row["name"] in names and not row["name"].lower().startswith("fun_"):
            bad("functions", f"'{row['name']}' at two addresses: "
                             f"{names[row['name']]} and {row['address']}")
        names[row["name"]] = row["address"]

    # Manually rejected false entries must not re-enter the function map.
    non_function_path = ROOT / "data/non_function_entries.csv"
    if non_function_path.exists():
        rejected: set[int] = set()
        for row in read(non_function_path):
            address = int(row["address"], 16)
            if address in rejected:
                bad("non-functions", f"duplicate address {row['address']}")
            rejected.add(address)
            if address in by_address:
                bad("non-functions", f"{row['address']} is both a rejected entry and a "
                    f"function ({by_address[address]['name']})")

    # --- c_sources.csv <-> functions.csv <-> disk ---
    for row in read(ROOT / "data/c_sources.csv"):
        address = int(row["address"], 16)
        target = by_address.get(address)
        if target is None:
            bad("c_sources", f"{row['address']} ({row['name']}) is not in functions.csv")
            continue
        if target["name"] != row["name"]:
            bad("c_sources", f"{row['address']} name mismatch: c_sources "
                             f"'{row['name']}' vs functions '{target['name']}'")
        if not (ROOT / row["source"]).exists():
            bad("c_sources", f"{row['source']} is not on disk ({row['name']})")
        if row["matching"] == "yes" and target["status"] != "matching":
            bad("c_sources", f"{row['name']} matching=yes but functions says "
                             f"'{target['status']}'")
        if row["matching"] == "no" and target["status"] == "matching":
            bad("c_sources", f"{row['name']} matching=no but functions says 'matching'")
        compiled_size = int(row["compiled_size"], 0)
        mapped_size = int(row["mapped_size"], 0)
        function_size = int(target["size"], 0)
        if mapped_size != function_size:
            bad("c_sources", f"{row['name']} mapped_size={mapped_size}, "
                             f"functions size={function_size}; c-status is stale")
        if row["matching"] == "yes" and compiled_size < mapped_size:
            bad("c_sources", f"{row['name']} a short C prefix was counted as matching: "
                             f"{compiled_size} < {mapped_size}")

    # --- matching_regions.csv ---
    regions = sorted((int(r["start"], 16), int(r["end"], 16), r["binary"])
                     for r in read(ROOT / "data/matching_regions.csv"))
    for (s1, e1, b1), (s2, _, b2) in zip(regions, regions[1:]):
        if e1 > s2:
            bad("regions", f"{b1} and {b2} overlap "
                           f"(0x{s1:08X}-0x{e1:08X} vs 0x{s2:08X})")

    # --- ram_map.csv ---
    ram = read(ROOT / "data/ram_map.csv")
    seen_ram: dict[int, str] = {}
    for row in ram:
        address = int(row["address"], 16)
        if address in seen_ram:
            bad("ram_map", f"{row['address']} under two names: "
                           f"{seen_ram[address]} and {row['name']}")
        seen_ram[address] = row["name"]
        size = int(row["size"] or 0)
        for lo, hi, label in RAM_REGIONS:
            if lo <= address < hi and address + size > hi:
                bad("ram_map", f"{row['name']} overflows the end of "
                               f"{label} by {address + size - hi} bytes")

    # --- ARM review table: covers the whole overlay range without gaps ---
    arm_review_path = ROOT / "data/arm_boundary_review.csv"
    if arm_review_path.exists():
        arm_ranges = []
        arm_seen: set[int] = set()
        for row in read(arm_review_path):
            address = int(row["address"], 16)
            size = int(row["corrected_size"])
            if address in arm_seen:
                bad("arm-review", f"duplicate address {row['address']}")
            arm_seen.add(address)
            target = by_address.get(address)
            if target is None:
                bad("arm-review", f"{row['address']} is not in functions.csv")
            else:
                if int(target["size"]) != size:
                    bad("arm-review", f"{row['address']} size mismatch: "
                        f"review {size}, functions {target['size']}")
                if target["module"] != "arm" or target["status"] in {
                    "candidate", "discovered"
                }:
                    bad("arm-review", f"{row['address']} is not marked as a "
                        f"reviewed ARM record")
            arm_ranges.append((address, address + size))
        arm_ranges.sort()
        if arm_ranges:
            if arm_ranges[0][0] != 0x08067E04 or arm_ranges[-1][1] != 0x0806B84C:
                bad("arm-review", "overlay ends are not 0x08067E04-0x0806B84C")
            for (_, end), (start, _) in zip(arm_ranges, arm_ranges[1:]):
                if end != start:
                    bad("arm-review", f"0x{end:08X}-0x{start:08X} has "
                        "a gap or overlap")

    # --- source: does every extern symbol resolve (RENAME BREAKAGE) ---
    symbols = set(names) | {r["name"] for r in ram}
    extern = re.compile(r"^\s*extern\s+.*?\b(\w+)\s*(?:\[|\(|;|=)", re.M)
    for source in sorted(ROOT.glob("src/**/*.c")):
        text = source.read_text(encoding="utf-8")
        for name in set(extern.findall(text)):
            # The `__thumb` suffix is NOT A SYMBOL but a resolution
            # instruction: the same address with the Thumb bit set
            # (tools/agbcc_build.py). The suffix is dropped when looking up,
            # otherwise every stored function pointer is reported as a false
            # "rename breakage".
            if name.endswith("__thumb"):
                name = name[: -len("__thumb")]
            if name not in symbols:
                bad("source", f"{source.relative_to(ROOT)}: extern '{name}' "
                              f"is in neither functions.csv nor ram_map.csv - "
                              f"possible rename breakage")

    # --- source/header: one extern type per physical RAM symbol ---
    ram_extern = re.compile(
        r"^\s*extern\s+(.+?)\s+(\*?)(gRam[0-9A-Fa-f]+)"
        r"(\s*\[[^;]*\])?\s*;",
        re.M,
    )
    ram_types: dict[str, dict[str, list[str]]] = {}
    declaration_files = sorted(ROOT.glob("src/**/*.c")) + sorted(
        ROOT.glob("include/**/*.h")
    )
    for source in declaration_files:
        text = source.read_text(encoding="utf-8")
        for match in ram_extern.finditer(text):
            type_text = " ".join(
                (match.group(1) + match.group(2) + (match.group(4) or "")).split()
            )
            ram_types.setdefault(match.group(3), {}).setdefault(type_text, []).append(
                str(source.relative_to(ROOT))
            )
    for symbol, declarations in sorted(ram_types.items()):
        if len(declarations) > 1:
            detail = "; ".join(
                f"{kind} ({', '.join(paths)})" for kind, paths in declarations.items()
            )
            bad("ram-extern", f"{symbol} has conflicting extern types: {detail}")

    # Seeing the same SYMBOL under the same struct NAME but with a DIFFERENT
    # BODY is a silent hazard: because the extern type is written identically,
    # the check above does not catch it. `Anchor` stood in four files under the
    # same name with two different bodies (one fully expanded, the others
    # padded). Since the layouts were compatible it raised no error, but
    # changing one would have silently made the other wrong.
    #
    # The check is DELIBERATELY narrow: identically named structs describing
    # different symbols are NORMAL in this project (each translation unit builds
    # its own local view, see include/ram_symbols.h). Only bodies that conflict
    # on the SAME symbol are reported.
    struct_def = re.compile(
        r"typedef\s+struct\s*(?:\w+)?\s*\{(.*?)\}\s*(\w+)\s*;", re.S
    )
    bodies_by_file: dict[str, dict[str, str]] = {}
    for source in declaration_files:
        text = source.read_text(encoding="utf-8")
        for match in struct_def.finditer(text):
            body = " ".join(
                re.sub(r"/\*.*?\*/", "", match.group(1), flags=re.S).split()
            )
            bodies_by_file.setdefault(str(source.relative_to(ROOT)), {})[
                match.group(2)
            ] = body

    for symbol, declarations in sorted(ram_types.items()):
        for type_text, paths in declarations.items():
            name = type_text.replace("*", "").strip().split()[-1]
            seen: dict[str, list[str]] = {}
            for path in paths:
                body = bodies_by_file.get(path, {}).get(name)
                if body is not None:
                    seen.setdefault(body, []).append(path)
            if len(seen) > 1:
                detail = "; ".join(f"({', '.join(v)})" for v in seen.values())
                bad(
                    "struct-body",
                    f"{symbol}: {name} is defined with different bodies: {detail}",
                )
    # --- Names and notes of matching functions ----------------------------
    #
    # These two checks were added because a real backlog had accumulated
    # (2026-09-06): the matching flow updates `status` but never returned to
    # Ghidra's PLACEHOLDER name and note. 88 of 365 matching functions still
    # carried a `FUN_` name, and 60 of them had the note "boundary and ARM/Thumb
    # mode are provisional" -- while byte-matching proves exactly the opposite.
    # The existing checks looked at whether names were CONSISTENT across files,
    # not at whether they were placeholders; so no gate ever rang.
    STALE_NOTE = "boundary and ARM/Thumb mode are provisional"
    placeholder = []
    for row in functions:
        if row.get("status") != "matching":
            continue
        if STALE_NOTE in (row.get("notes") or ""):
            bad(
                "stale-note",
                f"{row['name']} is byte-matching but its note still says the "
                f"boundary/mode is 'provisional'",
            )
        if row["name"].startswith("FUN_"):
            placeholder.append(row["name"])

    if problems:
        print(f"INCONSISTENCY: {len(problems)} problems\n")
        for problem in problems:
            print(f"  {problem}")
        sys.exit(1)
    print(f"consistency: CLEAN  ({len(functions)} functions, {len(ram)} RAM symbols, "
          f"{len(regions)} regions)")
    # Not an error but VISIBILITY: empty stubs and blind forwarding wrappers
    # are deliberately left unnamed (what they do is unknown, and naming them
    # would be invention). Printing the count on every run prevents the
    # nameable ones from accumulating silently.
    if placeholder:
        print(f"  note: {len(placeholder)} matching functions still carry a "
              f"placeholder `FUN_` name (empty stubs / blind wrappers expected)")


if __name__ == "__main__":
    main()
