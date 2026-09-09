#!/bin/sh
# A reproducible host-compiler wrapper that builds pret/agbcc's old GCC sources
# with modern macOS Clang. setup_agbcc.sh and the toolchain reproduction
# experiment take the same flags from this single file.
exec "${AGBCC_HOST_CC:-clang}" -std=gnu89 -fcommon \
  -Wno-implicit-function-declaration -Wno-implicit-int -Wno-int-conversion \
  -Wno-return-type -Wno-incompatible-pointer-types \
  -Wno-deprecated-non-prototype -Wno-parentheses \
  -Wno-shift-op-parentheses -Wno-dangling-else -Wno-format \
  -Wno-error "$@"
