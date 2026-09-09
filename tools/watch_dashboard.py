#!/usr/bin/env python3
"""Watch the data files and regenerate the dashboard JSON when they change.

The dashboard imports the JSON statically, so when the file changes Vite HMR
refreshes the page by itself. Run together with `make dashboard-dev`, the map
updates live as work progresses.

Usage:  python3 tools/watch_dashboard.py [--interval 1.0]
"""
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GENERATOR = ROOT / "tools/generate_dashboard_data.py"
WATCHED = [
    GENERATOR,  # also refresh when the labelling/clustering logic changes
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
        print(f"[{stamp}] ERROR: {result.stderr.strip()}", flush=True)


def main() -> None:
    interval = 1.0
    for arg in sys.argv[1:]:
        if arg.startswith("--interval"):
            interval = float(arg.split("=", 1)[1] if "=" in arg else 1.0)

    print(f"Dashboard watcher started ({len(WATCHED)} files + decompiler output).",
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
