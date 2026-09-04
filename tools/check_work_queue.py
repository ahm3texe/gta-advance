#!/usr/bin/env python3
"""data/work_queue.csv semasini ve odak kurallarini denetler."""

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
            print(f"HATA: work_queue basligi {FIELDS} olmali", file=sys.stderr)
            return 1
        rows = list(reader)

    errors = []
    ids = set()
    active = []
    for line, row in enumerate(rows, start=2):
        task_id = row["id"]
        if not re.fullmatch(r"[A-Z][A-Z0-9-]*-\d{3}", task_id):
            errors.append(f"satir {line}: gecersiz id {task_id!r}")
        if task_id in ids:
            errors.append(f"satir {line}: yinelenen id {task_id}")
        ids.add(task_id)
        if row["priority"] not in PRIORITIES:
            errors.append(f"satir {line}: gecersiz oncelik {row['priority']!r}")
        if row["status"] not in STATUSES:
            errors.append(f"satir {line}: gecersiz durum {row['status']!r}")
        if not row["title"].strip() or not row["acceptance"].strip():
            errors.append(f"satir {line}: baslik ve kabul olcutu zorunlu")
        if row["status"] == "done" and not row["evidence"].strip():
            errors.append(f"satir {line}: tamamlanan isin kaniti zorunlu")
        if row["status"] == "in_progress":
            active.append(task_id)

    if len(active) > 1:
        errors.append(f"ayni anda yalniz bir is aktif olabilir: {', '.join(active)}")

    if errors:
        print("IS KUYRUGU HATALI:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    active_label = active[0] if active else "yok"
    print(f"is kuyrugu: TEMIZ ({len(rows)} is, aktif: {active_label})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
