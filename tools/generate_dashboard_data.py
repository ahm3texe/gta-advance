#!/usr/bin/env python3
import csv
import json
import re
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS = ROOT / "data/functions.csv"
REGIONS = ROOT / "data/matching_regions.csv"
C_SOURCES = ROOT / "data/c_sources.csv"
LIBC_REGIONS = ROOT / "data/libc_regions.csv"
WORK_QUEUE = ROOT / "data/work_queue.csv"
BOUNDARY_BASELINE = ROOT / "data/boundary_baseline.json"
OUTPUT = ROOT / "dashboard/app/decomp-data.json"
DECOMPILER = ROOT / "analysis/decompiler"

# Ard arda gelen fonksiyonlar büyük olasılıkla aynı çeviri biriminden derlendi.
# Bu eşikten büyük boşluklar yeni bir küme başlatır.
CLUSTER_GAP = 512

MODULE_LABELS = {
    "bootstrap": "Başlangıç",
    "interrupt": "Kesme sistemi",
    "save": "Kayıt sistemi",
    "sdk": "GBA SDK",
    "serialization": "Serileştirme",
    "ui": "Arayüz",
    "libc": "C kitaplığı",
    "unknown": "Sınıflandırılmamış",
}


def read_decompiler_exports() -> dict[int, tuple[str, str]]:
    exports: dict[int, tuple[str, str]] = {}
    for path in DECOMPILER.glob("*.c"):
        code = path.read_text(encoding="utf-8")
        match = re.search(r"Function: .*? @ (?:0x)?([0-9A-Fa-f]{8})", code)
        if match:
            exports[int(match.group(1), 16)] = (
                str(path.relative_to(ROOT)),
                code,
            )
    return exports


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def matched_bytes(start: int, size: int, regions: list[tuple[int, int]]) -> int:
    """Fonksiyon aralığının doğrulanmış ROM bölgeleriyle kesişen byte sayısı."""
    end = start + size
    total = 0
    for region_start, region_end in regions:
        overlap = min(end, region_end) - max(start, region_start)
        if overlap > 0:
            total += overlap
    return total


def build_clusters(rows: list[dict]) -> None:
    """Sınıflandırılmamış fonksiyonları bitişiklik kümelerine ayır.

    Sınıflandırılmış fonksiyonlar kendi modül grubunda kalır; kümeleme yalnızca
    yapısı henüz bilinmeyen bölgeye sanal çeviri birimi sınırları getirir.
    """
    for row in rows:
        if row["module"] != "unknown":
            row["cluster"] = row["module"]
            row["clusterLabel"] = MODULE_LABELS.get(row["module"], row["module"])

    unknown = sorted(
        (row for row in rows if row["module"] == "unknown"),
        key=lambda row: row["_start"],
    )
    clusters: list[list[dict]] = []
    current: list[dict] = []
    previous_end = None
    for row in unknown:
        if previous_end is not None and row["_start"] - previous_end > CLUSTER_GAP:
            clusters.append(current)
            current = []
        current.append(row)
        previous_end = max(previous_end or 0, row["_start"] + row["size"])
    if current:
        clusters.append(current)

    for cluster in clusters:
        key = f"0x{cluster[0]['_start']:08X}"
        for row in cluster:
            row["cluster"] = key
            row["clusterLabel"] = key


def main() -> None:
    function_rows = read_csv(FUNCTIONS)
    region_rows = read_csv(REGIONS)
    # Kaynak turu ile eslesme durumu ayri eksenlerdir. Eslesmeyen C de C'dir;
    # kaynagi olmayan aday ise assembly degildir.
    c_source_rows = read_csv(C_SOURCES) if C_SOURCES.exists() else []
    c_sources = {row["address"].upper(): row for row in c_source_rows}
    c_matched = {
        row["address"].upper(): row["source"]
        for row in c_source_rows
        if row["matching"] == "yes"
    }
    status_counts = Counter(row["status"] for row in function_rows)
    decompiler_exports = read_decompiler_exports()

    verified_regions = [
        (int(row["start"], 0), int(row["end"], 0)) for row in region_rows
    ]

    functions = []
    source_texts: dict[str, str] = {}
    total_code_bytes = 0
    matching_code_bytes = 0
    for row in function_rows:
        size = int(row["size"], 0) if row["size"].strip() else 1
        start = int(row["address"], 0)
        total_code_bytes += size
        if row["status"] == "matching":
            matching_code_bytes += size
        verified = matched_bytes(start, size, verified_regions)
        c_source = c_sources.get(row["address"].upper())
        source_type = "c" if c_source else ("asm" if row["status"] == "matching" else "none")
        function = {
            "address": row["address"],
            "name": row["name"],
            "size": size,
            "status": row["status"],
            "module": row["module"],
            "notes": row["notes"],
            "matchedBytes": verified,
            "matchPercent": round(100 * verified / size, 2) if size else 0.0,
            "sourceType": source_type,
            "sourcePath": c_source["source"] if c_source else "",
            "cMatching": bool(c_source and c_source["matching"] == "yes"),
            "_start": start,
        }
        export = decompiler_exports.get(start)
        if export:
            function["analysisPath"], function["analysisCode"] = export
        # Kendi yazdigimiz kaynak: Ghidra ciktisindan farkli ve asil olan bu.
        source_path = function["sourcePath"]
        if source_path:
            function["sourceCode"] = source_texts.setdefault(
                source_path, (ROOT / source_path).read_text(encoding="utf-8")
                if (ROOT / source_path).exists() else "")
        functions.append(function)

    build_clusters(functions)
    for function in functions:
        del function["_start"]

    regions = []
    matching_region_bytes = 0
    for row in region_rows:
        start = int(row["start"], 0)
        end = int(row["end"], 0)
        size = end - start
        matching_region_bytes += size
        regions.append(
            {
                "start": row["start"],
                "end": row["end"],
                "size": size,
                "module": row["module"],
                "notes": row["notes"],
            }
        )

    # agbcc libc.a'ya karsi dogrulanmis standart kutuphane bolgeleri.
    libc_region_bytes = sum(
        int(row["end"], 16) - int(row["address"], 16)
        for row in (read_csv(LIBC_REGIONS) if LIBC_REGIONS.exists() else [])
    )
    cluster_count = len({function["cluster"] for function in functions})
    work_queue = read_csv(WORK_QUEUE) if WORK_QUEUE.exists() else []
    boundary_debt = 0
    if BOUNDARY_BASELINE.exists():
        boundary_debt = len(
            json.loads(BOUNDARY_BASELINE.read_text(encoding="utf-8"))["shortBoundaries"]
        )
    payload = {
        "summary": {
            "functionCount": len(functions),
            "reviewedCount": (
                status_counts["documented"]
                + status_counts["decompiled"]
                + status_counts["matching"]
            ),
            "matchingCount": status_counts["matching"],
            "totalCodeBytes": total_code_bytes,
            "matchingCodeBytes": matching_code_bytes,
            "matchingCodePercent": round(100 * matching_code_bytes / total_code_bytes, 2),
            "matchingRegionBytes": matching_region_bytes,
            "libcRegionBytes": libc_region_bytes,
            "verifiedRomBytes": matching_region_bytes + libc_region_bytes,
            "clusterCount": cluster_count,
            "cSourceCount": len(c_sources),
            "cMatchingCount": len(c_matched),
            "cSourceBytes": sum(
                f["size"] for f in functions if f["sourceType"] == "c"
            ),
            "boundaryDebtCount": boundary_debt,
        },
        "functions": functions,
        "regions": regions,
        "workQueue": work_queue,
    }

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(payload, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    print(f"Dashboard verisi: {len(functions)} fonksiyon, {cluster_count} küme, {OUTPUT}")


if __name__ == "__main__":
    main()
