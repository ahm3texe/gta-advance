#!/bin/sh
# Install the agbcc compiler (Nintendo's GCC 2.8.1 patched for the GBA).
#
# That the ROM was built with this compiler was verified at byte level on
# src/save/save_helpers.c; details in docs/COMPILER.md.
#
# The binaries do not enter the repository (8.8 MB); this script rebuilds them.
set -e

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${AGBCC_WORK:-"$ROOT/build/agbcc-src"}
AGBCC_REPO=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"]["repository"])' "$ROOT/config/toolchain.lock.json")
AGBCC_COMMIT=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"]["compatibleRevision"])' "$ROOT/config/toolchain.lock.json")

if [ -x "$ROOT/tools/agbcc/bin/agbcc" ] && [ "$1" != "--force" ]; then
    echo "agbcc is already installed: tools/agbcc/bin/agbcc"
    python3 "$ROOT/tools/verify_toolchain.py"
    echo "To reinstall: $0 --force"
    exit 0
fi

if ! command -v arm-none-eabi-as >/dev/null 2>&1 || ! command -v arm-none-eabi-ar >/dev/null 2>&1; then
    echo "ERROR: arm-none-eabi binutils not found." >&2
    echo "  brew install arm-none-eabi-binutils" >&2
    exit 1
fi

# agbcc is 1998-era C source; it does not build with modern clang's defaults.
# The flags live in a separate tracked file so that the setup script and the
# reproduction experiment use identical settings.
CCWRAP="$ROOT/tools/agbcc_host_cc.sh"

if [ ! -d "$WORK" ]; then
    echo "Fetching agbcc source..."
    git init "$WORK"
    git -C "$WORK" remote add origin "$AGBCC_REPO"
    git -C "$WORK" fetch --depth 1 origin "$AGBCC_COMMIT"
    git -C "$WORK" checkout --detach FETCH_HEAD
elif [ ! -d "$WORK/.git" ]; then
    echo "ERROR: $WORK exists but is not a Git checkout." >&2
    exit 1
elif [ "$(git -C "$WORK" rev-parse HEAD)" != "$AGBCC_COMMIT" ]; then
    echo "ERROR: $WORK is not at the expected agbcc revision." >&2
    echo "  expected: $AGBCC_COMMIT" >&2
    echo "  found:  $(git -C "$WORK" rev-parse HEAD)" >&2
    echo "Use a different, empty AGBCC_WORK directory." >&2
    exit 1
fi

echo "building agbcc (this may take a few minutes)..."
( cd "$WORK" && CC="$CCWRAP" CXX=clang++ ./build.sh )

echo "Installing into the project..."
( cd "$WORK" && ./install.sh "$ROOT" )
"$ROOT/tools/agbcc/bin/agbcc" --version 2>/dev/null || true
python3 "$ROOT/tools/verify_toolchain.py" --corpus
echo "Done: tools/agbcc/bin/{agbcc,old_agbcc,agbcc_arm}"
