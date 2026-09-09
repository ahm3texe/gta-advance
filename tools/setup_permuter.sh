#!/usr/bin/env bash
# decomp-permuter-agbcc setup (macOS).
#
# WHY: for functions one byte away, such as ClearTextArea, we were searching
# permutations by hand (5040 declaration permutations were tried in one file).
# This tool automates that search.
#
# DISCIPLINE NOTE (docs/WORKFLOW.md §6): the permuter is a HYPOTHESIS GENERATOR.
# Once a winning form is found, WHY it works must be understood and written into
# docs/COMPILER.md as a rule. An unexplained match is not accepted.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXT="$ROOT/tools/external"
REV=1f7ef872b12f54db7678ff00e0346abc015410ae   # pinned revision
PY312="$HOME/.pyenv/versions/3.12.3/bin/python3.12"

mkdir -p "$EXT"

# 1. The source, at a fixed revision.
if [ ! -d "$EXT/permuter/.git" ]; then
  git clone --quiet https://github.com/WhenGryphonsFly/decomp-permuter-agbcc.git "$EXT/permuter"
fi
git -C "$EXT/permuter" checkout --quiet "$REV"

# 2. An isolated environment. ensurepip is broken on the system Python 3.14, so
#    pyenv 3.12 is used.
[ -x "$PY312" ] || { echo "pyenv 3.12.3 missing: pyenv install 3.12.3"; exit 1; }
[ -d "$EXT/venv" ] || "$PY312" -m venv "$EXT/venv"
# pycparser 3.x removed plyparser; the tool expects 2.x.
"$EXT/venv/bin/pip" install --quiet "pycparser<3" toml

# 3. macOS patch: upstream looks for homebrew's GNU `cpp-*` binary. Here it
#    is absent; the target chain's preprocessor is already the right choice
#    (the same ARM EABI definitions as agbcc). gmake is not installed either,
#    and the system make is sufficient.
python3 - "$EXT/permuter/import.py" <<'PY'
import sys, pathlib
p = pathlib.Path(sys.argv[1]); s = p.read_text()
old = 'cpp_cmd = homebrew_gcc_cpp() if is_macos else "cpp"\nmake_cmd = "gmake" if is_macos else "make"'
new = 'cpp_cmd = "arm-none-eabi-cpp"\nmake_cmd = "make"'
if old in s:
    p.write_text(s.replace(old, new, 1)); print("  import.py patched")
PY

echo "Setup complete."
echo
echo "To take on a function:"
echo "  1) build the target .o from the ROM bytes -- see build/permuter/target.s."
echo "     THE MAPPING SYMBOLS ARE CRITICAL: code body \$t, literal pool \$d."
echo "     Leaving the pool inside \$t makes objdump decode it as CODE; the target"
echo "     then shows 66 instructions (really 61) and the permuter chases the WRONG target."
echo "     Where the pool starts is visible in build/cmatch/<name>.s: it is the"
echo "     .align + .word block that follows the last instruction."
echo "  2) cd tools/external/permuter && ../venv/bin/python import.py \\"
echo "       <source.c> <target.o> <FunctionName>"
echo "  3) AFTER THE IMPORT two manual fixes are needed:"
echo "     - __inline__ -> inline inside nonmatchings/<Name>/base.c"
echo "       (pycparser does not know the GCC keyword)"
echo "     - the realpath \"\$3\" line in nonmatchings/<Name>/compile.sh"
echo "       (BSD realpath fails on a path that does not exist; the output path stays empty)"
echo "  4) ../venv/bin/python permuter.py nonmatchings/<Name> --stop-on-zero -j 4"
echo
echo "WARNING -- DO NOT TRUST INTERMEDIATE SCORES. The permuter's score is an"
echo "instruction-weighted heuristic, NOT our criterion (exact byte equality). Measured:"
echo "in CleanupAreaTiles a score of 50 -> 45 was called a 'new best', but the"
echo "byte difference STAYED at 7; only its position moved (0x1a -> 0x1c)."
echo "The ONLY meaningful value is a score of 0; that means an exact match."
echo "Every candidate must be verified with our own measurement before acceptance:"
echo "  make c-match FILE=<source.c>"
