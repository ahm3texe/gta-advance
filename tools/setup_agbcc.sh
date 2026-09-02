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

if [ -x "$ROOT/tools/agbcc/bin/agbcc" ] && [ "$1" != "--force" ]; then
    echo "agbcc zaten kurulu: tools/agbcc/bin/agbcc"
    echo "Yeniden kurmak icin: $0 --force"
    exit 0
fi

if ! command -v arm-none-eabi-as >/dev/null 2>&1 || ! command -v arm-none-eabi-ar >/dev/null 2>&1; then
    echo "HATA: arm-none-eabi binutils bulunamadi." >&2
    echo "  brew install arm-none-eabi-binutils" >&2
    exit 1
fi

# agbcc 1998 donemi C kaynagi; modern clang'in varsayilanlariyla derlenmez.
CCWRAP="$WORK/../agbcc-cc"
mkdir -p "$(dirname "$CCWRAP")"
cat > "$CCWRAP" <<'WRAP'
#!/bin/sh
exec ${AGBCC_HOST_CC:-clang} -std=gnu89 -fcommon \
  -Wno-implicit-function-declaration -Wno-implicit-int -Wno-int-conversion \
  -Wno-return-type -Wno-incompatible-pointer-types -Wno-deprecated-non-prototype \
  -Wno-parentheses -Wno-shift-op-parentheses -Wno-dangling-else -Wno-format \
  -Wno-error "$@"
WRAP
chmod +x "$CCWRAP"

if [ ! -d "$WORK" ]; then
    echo "agbcc kaynagi aliniyor..."
    git clone --depth 1 https://github.com/pret/agbcc.git "$WORK"
fi

echo "agbcc derleniyor (birkac dakika surebilir)..."
( cd "$WORK" && CC="$CCWRAP" CXX=clang++ ./build.sh )

echo "Projeye kuruluyor..."
( cd "$WORK" && ./install.sh "$ROOT" )
"$ROOT/tools/agbcc/bin/agbcc" --version 2>/dev/null || true
echo "Tamam: tools/agbcc/bin/{agbcc,old_agbcc,agbcc_arm}"
