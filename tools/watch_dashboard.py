#!/usr/bin/env python3
"""Veri dosyalarini izler ve degistiginde dashboard JSON'unu yeniden uretir.

Dashboard JSON'u statik olarak import ediyor, bu yuzden dosya degisince Vite
HMR sayfayi kendiliginden yeniliyor. Yani `make dashboard-dev` ile birlikte
calistirildiginde harita, calisma ilerledikce anlik guncellenir.

Kullanim:  python3 tools/watch_dashboard.py [--interval 1.0]
"""
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GENERATOR = ROOT / "tools/generate_dashboard_data.py"
WATCHED = [
    GENERATOR,  # etiket/kumeleme mantigi degisince de yenile
    ROOT / "data/functions.csv",
    ROOT / "data/function_overrides.csv",
    ROOT / "data/matching_regions.csv",
    ROOT / "data/c_sources.csv",
    ROOT / "data/ram_map.csv",
]
WATCHED_DIRS = [ROOT / "analysis/decompiler"]


def fingerprint() -> tuple:
    stamps = []
    for path in WATCHED:
        stamps.append((str(path), path.stat().st_mtime_ns if path.exists() else 0))
    for directory in WATCHED_DIRS:
        if directory.exists():
            for path in sorted(directory.glob("*.c")):
                stamps.append((str(path), path.stat().st_mtime_ns))
    return tuple(stamps)


def regenerate() -> None:
    result = subprocess.run(
        [sys.executable, str(GENERATOR)], capture_output=True, text=True
    )
    stamp = time.strftime("%H:%M:%S")
    if result.returncode == 0:
        print(f"[{stamp}] {result.stdout.strip()}", flush=True)
    else:
        print(f"[{stamp}] HATA: {result.stderr.strip()}", flush=True)


def main() -> None:
    interval = 1.0
    for arg in sys.argv[1:]:
        if arg.startswith("--interval"):
            interval = float(arg.split("=", 1)[1] if "=" in arg else 1.0)

    print(f"Dashboard izleyici basladi ({len(WATCHED)} dosya + decompiler ciktisi).",
          flush=True)
    regenerate()
    previous = fingerprint()
    while True:
        time.sleep(interval)
        current = fingerprint()
        if current != previous:
            previous = current
            regenerate()


if __name__ == "__main__":
    main()
