#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
project_dir="$repo_dir/analysis/ghidra-project"
report_path="$repo_dir/analysis/function_map_ghidra.csv"

"$repo_dir/tools/prepare_rom.sh" --verify
mkdir -p "$project_dir" "$repo_dir/analysis"

"$repo_dir/tools/ghidra_headless.sh" \
    "$project_dir" gta_advance \
    -import "$repo_dir/baserom.gba" \
    -overwrite \
    -loader BinaryLoader \
    -loader-baseAddr 0x08000000 \
    -processor ARM:LE:32:v4t \
    -scriptPath "$repo_dir/tools/ghidra" \
    -preScript BootstrapGba.java \
    -postScript ExportFunctionMap.java "$report_path" \
    -analysisTimeoutPerFile 900

echo "Fonksiyon raporu: $report_path"

