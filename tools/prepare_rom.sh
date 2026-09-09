#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
rom_path="$repo_dir/baserom.gba"
hash_file="$repo_dir/config/rom.sha1"

verify_rom() {
    test -f "$rom_path" || {
        echo "Eksik: $rom_path" >&2
        return 1
    }
    (cd "$repo_dir" && shasum -a 1 -c "$hash_file")
}

if test "${1-}" = "--verify"; then
    verify_rom
    exit
fi

archive=${1-}
test -n "$archive" && test -f "$archive" || {
    echo "Usage: $0 /full/path/game.zip" >&2
    exit 2
}

entry=$(unzip -Z1 "$archive" | awk 'tolower($0) ~ /\.gba$/ { print; exit }')
test -n "$entry" || {
    echo "No .gba file found in the archive." >&2
    exit 1
}

tmp_path=$(mktemp "$repo_dir/.baserom.gba.XXXXXX")
trap 'rm -f "$tmp_path"' EXIT HUP INT TERM
unzip -p "$archive" "$entry" > "$tmp_path"
mv "$tmp_path" "$rom_path"
trap - EXIT HUP INT TERM

verify_rom
echo "Ready: $rom_path (ignored by Git)"

