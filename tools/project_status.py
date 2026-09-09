#!/usr/bin/env python3
"""Generate the reproducible project status from the data sources."""

from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
OUTPUT = ROOT / "docs/STATUS.md"


def rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def snapshot() -> dict:
    functions = rows(DATA / "functions.csv")
    c_sources = rows(DATA / "c_sources.csv")
    regions = rows(DATA / "matching_regions.csv")
    libc = rows(DATA / "libc_regions.csv")
    queue = rows(DATA / "work_queue.csv")
    boundary = json.loads((DATA / "boundary_baseline.json").read_text(encoding="utf-8"))
    toolchain = json.loads((ROOT / "config/toolchain.lock.json").read_text(encoding="utf-8"))
    counts = Counter(row["status"] for row in functions)
    total_bytes = sum(int(row["size"], 0) for row in functions if row["size"].strip())
    matching_bytes = sum(
        int(row["size"], 0)
        for row in functions
        if row["status"] == "matching" and row["size"].strip()
    )
    region_bytes = sum(int(row["end"], 0) - int(row["start"], 0) for row in regions)
    libc_bytes = sum(int(row["end"], 0) - int(row["address"], 0) for row in libc)
    return {
        "functionCount": len(functions),
        "functionBytes": total_bytes,
        "reviewedCount": counts["documented"] + counts["decompiled"] + counts["matching"],
        "matchingCount": counts["matching"],
        "matchingBytes": matching_bytes,
        "matchingPercent": 100 * matching_bytes / total_bytes if total_bytes else 0,
        "cSourceCount": len(c_sources),
        "cMatchingCount": sum(row["matching"] == "yes" for row in c_sources),
        "sourceRegionBytes": region_bytes,
        "libcBytes": libc_bytes,
        "verifiedRomBytes": region_bytes + libc_bytes,
        "boundaryDebt": len(boundary["shortBoundaries"]),
        "boundarySkippedArm": len(boundary["skippedArm"]),
        "boundarySkippedOversized": len(boundary["skippedOversized"]),
        "queue": queue,
        "toolchainRevision": toolchain["source"]["compatibleRevision"],
        "toolchainCorpus": toolchain["compiler"]["corpusSha256"],
    }


def markdown(state: dict) -> str:
    active = [row for row in state["queue"] if row["status"] == "in_progress"]
    open_tasks = [row for row in state["queue"] if row["status"] in {"in_progress", "todo", "blocked"}]
    task_lines = []
    for task in open_tasks:
        task_lines.append(
            f"| {task['id']} | {task['priority']} | {task['status']} | "
            f"{task['title']} | {task['acceptance']} |"
        )
    active_text = (
        f"**{active[0]['id']} — {active[0]['title']}**" if active else "No active task."
    )
    return f"""# Current project status

This file is not edited by hand. `make status-update` generates it from
`data/*.csv`, the boundary baseline and the toolchain lock. For a live terminal
summary, run `make status`.

## Measurements

| Measurement | Value |
|---|---:|
| Function map | {state['functionCount']} functions / {state['functionBytes']} bytes |
| Human review (`documented+`) | {state['reviewedCount']} / {state['functionCount']} |
| Byte-matching | {state['matchingCount']} functions / {state['matchingBytes']} bytes ({state['matchingPercent']:.2f}%) |
| C sources | {state['cSourceCount']} total / {state['cMatchingCount']} matching |
| ROM verified from source | {state['sourceRegionBytes']} bytes |
| libc verification | {state['libcBytes']} bytes |
| Total verified ROM area | {state['verifiedRomBytes']} bytes |
| Open boundary debt | {state['boundaryDebt']} short + {state['boundarySkippedArm']} ARM review + {state['boundarySkippedOversized']} oversized |

## The single active task

{active_text}

## Open work queue

| ID | Priority | Status | Task | Acceptance criteria |
|---|---|---|---|---|
{chr(10).join(task_lines)}

## Toolchain lock

- Compatible pret/agbcc revision: `{state['toolchainRevision']}`
- Fixed representative C-corpus fingerprint: `{state['toolchainCorpus']}`
- The ROM output is a hybrid integration test; unknown bytes are copied from the base ROM.
"""


def terminal(state: dict) -> None:
    active = next((row for row in state["queue"] if row["status"] == "in_progress"), None)
    print(f"Functions:        {state['functionCount']} / {state['functionBytes']} bytes")
    print(f"Reviewed:         {state['reviewedCount']}")
    print(f"Byte-matching:    {state['matchingCount']} / {state['matchingBytes']} bytes "
          f"({state['matchingPercent']:.2f}%)")
    print(f"C sources:        {state['cSourceCount']} ({state['cMatchingCount']} matching)")
    print(f"Verified ROM:     {state['verifiedRomBytes']} bytes")
    print(f"Boundary debt:    {state['boundaryDebt']} short")
    print(f"Active task:      {active['id'] + ' — ' + active['title'] if active else 'none'}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.write and args.check:
        parser.error("--write and --check cannot be used together")

    state = snapshot()
    content = markdown(state)
    if args.write:
        OUTPUT.write_text(content, encoding="utf-8")
        print(f"Status document updated: {OUTPUT.relative_to(ROOT)}")
        return 0
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != content:
            print("ERROR: docs/STATUS.md is stale; run `make status-update`.", file=sys.stderr)
            return 1
        print("status document: CLEAN")
        return 0
    terminal(state)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
