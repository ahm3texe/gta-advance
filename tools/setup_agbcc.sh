#!/bin/sh
# agbcc (Nintendo'nun GBA icin yamalanmis GCC 2.8.1) derleyicisini kurar.
#
# ROM'un bu derleyiciyle uretildigi src/save/save_helpers.c uzerinde
# byte duzeyinde dogrulandi; ayrinti docs/COMPILER.md icinde.
#
# Ikili dosyalar depoya girmez (8.8 MB); bu betik onlari yeniden uretir.
set -e

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${AGBCC_WORK:-"$ROOT/build/agbcc-src"}
AGBCC_REPO=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"]["repository"])' "$ROOT/config/toolchain.lock.json")
AGBCC_COMMIT=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"]["compatibleRevision"])' "$ROOT/config/toolchain.lock.json")

if [ -x "$ROOT/tools/agbcc/bin/agbcc" ] && [ "$1" != "--force" ]; then
    echo "agbcc zaten kurulu: tools/agbcc/bin/agbcc"
    python3 "$ROOT/tools/verify_toolchain.py"
    echo "Yeniden kurmak icin: $0 --force"
    exit 0
fi

if ! command -v arm-none-eabi-as >/dev/null 2>&1 || ! command -v arm-none-eabi-ar >/dev/null 2>&1; then
    echo "HATA: arm-none-eabi binutils bulunamadi." >&2
    echo "  brew install arm-none-eabi-binutils" >&2
    exit 1
fi

# agbcc 1998 donemi C kaynagi; modern clang'in varsayilanlariyla derlenmez.
# Bayraklar ayri bir izlenen dosyada tutulur ki kurulum betigi ile yeniden
# uretme deneyi birbirinden sapmasin.
CCWRAP="$ROOT/tools/agbcc_host_cc.sh"

if [ ! -d "$WORK" ]; then
    echo "agbcc kaynagi aliniyor..."
    git init "$WORK"
    git -C "$WORK" remote add origin "$AGBCC_REPO"
    git -C "$WORK" fetch --depth 1 origin "$AGBCC_COMMIT"
    git -C "$WORK" checkout --detach FETCH_HEAD
elif [ ! -d "$WORK/.git" ]; then
    echo "HATA: $WORK var ama bir Git checkout'u degil." >&2
    exit 1
elif [ "$(git -C "$WORK" rev-parse HEAD)" != "$AGBCC_COMMIT" ]; then
    echo "HATA: $WORK beklenen agbcc revizyonunda degil." >&2
    echo "  beklenen: $AGBCC_COMMIT" >&2
    echo "  bulunan:  $(git -C "$WORK" rev-parse HEAD)" >&2
    echo "Farkli bos bir AGBCC_WORK dizini kullanin." >&2
    exit 1
fi

echo "agbcc derleniyor (birkac dakika surebilir)..."
( cd "$WORK" && CC="$CCWRAP" CXX=clang++ ./build.sh )

echo "Projeye kuruluyor..."
( cd "$WORK" && ./install.sh "$ROOT" )
"$ROOT/tools/agbcc/bin/agbcc" --version 2>/dev/null || true
python3 "$ROOT/tools/verify_toolchain.py" --corpus
echo "Tamam: tools/agbcc/bin/{agbcc,old_agbcc,agbcc_arm}"
