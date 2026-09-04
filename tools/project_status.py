#!/usr/bin/env python3
"""Tekrarlanabilir proje durumunu veri kaynaklarindan uretir."""

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
        f"**{active[0]['id']} — {active[0]['title']}**" if active else "Aktif iş yok."
    )
    return f"""# Güncel proje durumu

Bu dosya elle düzenlenmez. `make status-update` ile `data/*.csv`, sınır
baseline'ı ve toolchain kilidinden üretilir. Canlı terminal özeti: `make status`.

## Ölçümler

| Ölçüm | Değer |
|---|---:|
| Fonksiyon haritası | {state['functionCount']} fonksiyon / {state['functionBytes']} bayt |
| İnsan incelemesi (`documented+`) | {state['reviewedCount']} / {state['functionCount']} |
| Byte-matching | {state['matchingCount']} fonksiyon / {state['matchingBytes']} bayt (%{state['matchingPercent']:.2f}) |
| C kaynağı | {state['cSourceCount']} toplam / {state['cMatchingCount']} matching |
| Kaynaktan doğrulanan ROM | {state['sourceRegionBytes']} bayt |
| libc doğrulaması | {state['libcBytes']} bayt |
| Toplam doğrulanmış ROM alanı | {state['verifiedRomBytes']} bayt |
| Açık sınır borcu | {state['boundaryDebt']} kısa sınır + {state['boundarySkippedArm']} ARM incelemesi + {state['boundarySkippedOversized']} aşırı büyüme |

## Şu anki tek aktif iş

{active_text}

## Açık iş kuyruğu

| ID | Öncelik | Durum | İş | Bitti sayılma koşulu |
|---|---|---|---|---|
{chr(10).join(task_lines)}

## Araç zinciri kilidi

- Uyumlu pret/agbcc revizyonu: `{state['toolchainRevision']}`
- Sabit temsil C-corpus parmak izi: `{state['toolchainCorpus']}`
- ROM çıktısı hibrit bütünleştirme sınamasıdır; bilinmeyen baytlar baserom'dan kopyalanır.
"""


def terminal(state: dict) -> None:
    active = next((row for row in state["queue"] if row["status"] == "in_progress"), None)
    print(f"Fonksiyon:        {state['functionCount']} / {state['functionBytes']} bayt")
    print(f"İncelenmiş:       {state['reviewedCount']}")
    print(f"Byte-matching:    {state['matchingCount']} / {state['matchingBytes']} bayt "
          f"(%{state['matchingPercent']:.2f})")
    print(f"C kaynağı:        {state['cSourceCount']} ({state['cMatchingCount']} matching)")
    print(f"Doğrulanmış ROM:  {state['verifiedRomBytes']} bayt")
    print(f"Sınır borcu:      {state['boundaryDebt']} kısa sınır")
    print(f"Aktif iş:         {active['id'] + ' — ' + active['title'] if active else 'yok'}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.write and args.check:
        parser.error("--write ve --check birlikte kullanilamaz")

    state = snapshot()
    content = markdown(state)
    if args.write:
        OUTPUT.write_text(content, encoding="utf-8")
        print(f"Durum belgesi güncellendi: {OUTPUT.relative_to(ROOT)}")
        return 0
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != content:
            print("HATA: docs/STATUS.md bayat; `make status-update` çalıştırın.", file=sys.stderr)
            return 1
        print("durum belgesi: TEMIZ")
        return 0
    terminal(state)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
