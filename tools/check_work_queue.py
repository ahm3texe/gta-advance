#!/usr/bin/env python3
"""Check the schema and focus rules of data/work_queue.csv."""

import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
QUEUE = ROOT / "data/work_queue.csv"
FIELDS = ["id", "priority", "area", "status", "title", "acceptance", "evidence", "updated"]
PRIORITIES = {"P0", "P1", "P2", "P3"}
STATUSES = {"todo", "in_progress", "blocked", "done"}


def main() -> int:
    with QUEUE.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != FIELDS:
            print(f"ERROR: the work_queue header must be {FIELDS}", file=sys.stderr)
            return 1
        rows = list(reader)

    errors = []
    ids = set()
    active = []
    for line, row in enumerate(rows, start=2):
        task_id = row["id"]
        if not re.fullmatch(r"[A-Z][A-Z0-9-]*-\d{3}", task_id):
            errors.append(f"line {line}: invalid id {task_id!r}")
        if task_id in ids:
            errors.append(f"line {line}: duplicate id {task_id}")
        ids.add(task_id)
        if row["priority"] not in PRIORITIES:
            errors.append(f"line {line}: invalid priority {row['priority']!r}")
        if row["status"] not in STATUSES:
            errors.append(f"line {line}: invalid status {row['status']!r}")
        if not row["title"].strip() or not row["acceptance"].strip():
            errors.append(f"line {line}: title and acceptance criteria are required")
        if row["status"] == "done" and not row["evidence"].strip():
            errors.append(f"line {line}: evidence is required for a completed task")
        if row["status"] == "in_progress":
            active.append(task_id)

    if len(active) > 1:
        errors.append(f"only one task may be active at a time: {', '.join(active)}")

    if errors:
        print("WORK QUEUE INVALID:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    active_label = active[0] if active else "none"
    print(f"work queue: CLEAN ({len(rows)} tasks, active: {active_label})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
