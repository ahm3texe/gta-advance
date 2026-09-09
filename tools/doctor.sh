#!/bin/sh
set -eu

printf '%-24s %s\n' TOOL STATUS
for tool in git make python3 unzip shasum arm-none-eabi-gcc arm-none-eabi-as arm-none-eabi-objdump ghidraRun; do
    if command -v "$tool" >/dev/null 2>&1; then
        printf '%-24s %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '%-24s %s\n' "$tool" MISSING
    fi
done

if test -x /Applications/mGBA.app/Contents/MacOS/mGBA; then
    printf '%-24s %s\n' mGBA /Applications/mGBA.app
elif command -v mgba >/dev/null 2>&1; then
    printf '%-24s %s\n' mGBA "$(command -v mgba)"
else
    printf '%-24s %s\n' mGBA MISSING
fi

if command -v brew >/dev/null 2>&1 && test -x "$(brew --prefix openjdk@21 2>/dev/null)/bin/java"; then
    printf '%-24s %s\n' java-21 "$(brew --prefix openjdk@21)"
elif command -v java >/dev/null 2>&1 && java -version >/dev/null 2>&1; then
    printf '%-24s %s\n' java "$(command -v java)"
else
    printf '%-24s %s\n' java-21 MISSING
fi

if test -x "$(dirname "$0")/agbcc/bin/agbcc"; then
    printf '%-24s %s\n' agbcc "tools/agbcc/bin/agbcc"
else
    printf '%-24s %s\n' agbcc "MISSING (make agbcc)"
fi

if test -f "$(dirname "$0")/../baserom.gba"; then
    "$(dirname "$0")/prepare_rom.sh" --verify
else
    echo "baserom.gba: NOT READY"
fi
