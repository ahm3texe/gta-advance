#!/bin/sh
# pret/agbcc'nin eski GCC kaynaklarini modern macOS Clang ile derleyen
# tekrarlanabilir host-compiler sarmalayicisi. setup_agbcc.sh ve toolchain
# yeniden-uretme deneyi ayni bayraklari bu tek dosyadan kullanir.
exec "${AGBCC_HOST_CC:-clang}" -std=gnu89 -fcommon \
  -Wno-implicit-function-declaration -Wno-implicit-int -Wno-int-conversion \
  -Wno-return-type -Wno-incompatible-pointer-types \
  -Wno-deprecated-non-prototype -Wno-parentheses \
  -Wno-shift-op-parentheses -Wno-dangling-else -Wno-format \
  -Wno-error "$@"
