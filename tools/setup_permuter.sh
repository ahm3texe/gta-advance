#!/usr/bin/env bash
# decomp-permuter-agbcc setup (macOS).
#
# WHY: for functions one byte away, such as ClearTextArea, we were searching
# permutations by hand (5040 declaration permutations were tried in one file).
# This tool automates that search.
#
# DISCIPLINE NOTE (docs/WORKFLOW.md 6): the permuter is a HYPOTHESIS GENERATOR.
# Once a winning form is found, WHY it works must be understood and written into
# docs/COMPILER.md as a rule. An unexplained match is not accepted.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXT="$ROOT/tools/external"
REV=1f7ef872b12f54db7678ff00e0346abc015410ae   # sabit revizyon
PY312="$HOME/.pyenv/versions/3.12.3/bin/python3.12"

mkdir -p "$EXT"

# 1. The source, at a fixed revision.
if [ ! -d "$EXT/permuter/.git" ]; then
  git clone --quiet https://github.com/WhenGryphonsFly/decomp-permuter-agbcc.git "$EXT/permuter"
fi
git -C "$EXT/permuter" checkout --quiet "$REV"

# 2. Izole ortam. Sistem Python 3.14'te ensurepip bozuk, pyenv 3.12 kullaniliyor.
[ -x "$PY312" ] || { echo "pyenv 3.12.3 missing: pyenv install 3.12.3"; exit 1; }
[ -d "$EXT/venv" ] || "$PY312" -m venv "$EXT/venv"
# pycparser 3.x removed plyparser; the tool expects 2.x.
"$EXT/venv/bin/pip" install --quiet "pycparser<3" toml

# 3. macOS yamasi: ust akis homebrew'un GNU `cpp-*` ikilisini ariyor. Bizde o
#    is absent; the target chain's preprocessor is already the right choice
#    (the same ARM EABI definitions as agbcc). gmake is not installed either,
#    and the system make is sufficient.
python3 - "$EXT/permuter/import.py" <<'PY'
import sys, pathlib
p = pathlib.Path(sys.argv[1]); s = p.read_text()
old = 'cpp_cmd = homebrew_gcc_cpp() if is_macos else "cpp"\nmake_cmd = "gmake" if is_macos else "make"'
new = 'cpp_cmd = "arm-none-eabi-cpp"\nmake_cmd = "make"'
if old in s:
    p.write_text(s.replace(old, new, 1)); print("  import.py yamalandi")
PY

echo "Kurulum tamam."
echo
echo "To take on a function:"
echo "  1) build the target .o from the ROM bytes -- see build/permuter/target.s."
echo "     THE MAPPING SYMBOLS ARE CRITICAL: code body \$t, literal pool \$d."
echo "     Havuzu \$t icinde birakmak objdump'a onu KOMUT cozumletir; hedef"
echo "     66 komut gorunur (gercek 61) ve permuter YANLIS hedefe calisir."
echo "     Havuzun nerede basladigi build/cmatch/<ad>.s icinde: son `bx`"
echo "     komutundan sonraki .align + .word blogu."
echo "  2) cd tools/external/permuter && ../venv/bin/python import.py \\"
echo "       <source.c> <target.o> <FunctionName>"
echo "  3) ICE AKTARIMDAN SONRA iki elle duzeltme gerekiyor:"
echo "     - nonmatchings/<Ad>/base.c icinde __inline__ -> inline"
echo "       (pycparser GCC anahtar sozcugunu tanimiyor)"
echo "     - nonmatchings/<Ad>/compile.sh icindeki realpath \"\$3\" satiri"
echo "       (BSD realpath var OLMAYAN yolda basarisiz; cikti yolu bos kaliyor)"
echo "  4) ../venv/bin/python permuter.py nonmatchings/<Ad> --stop-on-zero -j 4"
echo
echo "UYARI -- ARA SKORLARA GUVENME. Permuter'in skoru KOMUT AGIRLIKLI bir"
echo "is heuristic, NOT our criterion (exact byte equality). Measured:"
echo "in CleanupAreaTiles a score of 50 -> 45 was called a 'new best', but the"
echo "farki 7'de KALDI; yalnizca farkin yeri kaydi (0x1a -> 0x1c)."
echo "The ONLY meaningful value is a score of 0; that means an exact match."
echo "Every candidate must be verified with our own measurement before acceptance:"
echo "  make c-match FILE=<source.c>"
