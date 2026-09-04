#!/usr/bin/env bash
# decomp-permuter-agbcc kurulumu (macOS).
#
# NEDEN: ClearTextArea gibi tek-bayt farkli fonksiyonlarda elle permutasyon
# aramasi yapiyorduk (bir dosyada 5040 bildirim permutasyonu denenmisti).
# Bu arac o aramayi otomatiklestirir.
#
# DISIPLIN NOTU (docs/WORKFLOW.md 6): permuter bir HIPOTEZ URETECIDIR.
# Kazanan bicim bulununca NEDEN calistigi anlasilip docs/COMPILER.md'ye kural
# olarak yazilmali. Anlasilmamis bir eslesme kabul edilmez.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXT="$ROOT/tools/external"
REV=1f7ef872b12f54db7678ff00e0346abc015410ae   # sabit revizyon
PY312="$HOME/.pyenv/versions/3.12.3/bin/python3.12"

mkdir -p "$EXT"

# 1. Kaynak, sabit revizyonda.
if [ ! -d "$EXT/permuter/.git" ]; then
  git clone --quiet https://github.com/WhenGryphonsFly/decomp-permuter-agbcc.git "$EXT/permuter"
fi
git -C "$EXT/permuter" checkout --quiet "$REV"

# 2. Izole ortam. Sistem Python 3.14'te ensurepip bozuk, pyenv 3.12 kullaniliyor.
[ -x "$PY312" ] || { echo "pyenv 3.12.3 yok: pyenv install 3.12.3"; exit 1; }
[ -d "$EXT/venv" ] || "$PY312" -m venv "$EXT/venv"
# pycparser 3.x plyparser'i kaldirdi; arac 2.x bekliyor.
"$EXT/venv/bin/pip" install --quiet "pycparser<3" toml

# 3. macOS yamasi: ust akis homebrew'un GNU `cpp-*` ikilisini ariyor. Bizde o
#    yok; hedef zincirin on islemcisi zaten dogru secim (agbcc ile ayni ARM
#    EABI tanimlari). gmake de kurulu degil, sistem make'i yeterli.
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
echo "Bir fonksiyonu almak icin:"
echo "  1) hedef .o'yu ROM baytlarindan uret -- bkz. build/permuter/target.s."
echo "     ESLEME SEMBOLLERI KRITIK: kod govdesi \$t, literal havuz \$d."
echo "     Havuzu \$t icinde birakmak objdump'a onu KOMUT cozumletir; hedef"
echo "     66 komut gorunur (gercek 61) ve permuter YANLIS hedefe calisir."
echo "     Havuzun nerede basladigi build/cmatch/<ad>.s icinde: son `bx`"
echo "     komutundan sonraki .align + .word blogu."
echo "  2) cd tools/external/permuter && ../venv/bin/python import.py \\"
echo "       <kaynak.c> <hedef.o> <FonksiyonAdi>"
echo "  3) ICE AKTARIMDAN SONRA iki elle duzeltme gerekiyor:"
echo "     - nonmatchings/<Ad>/base.c icinde __inline__ -> inline"
echo "       (pycparser GCC anahtar sozcugunu tanimiyor)"
echo "     - nonmatchings/<Ad>/compile.sh icindeki realpath \"\$3\" satiri"
echo "       (BSD realpath var OLMAYAN yolda basarisiz; cikti yolu bos kaliyor)"
echo "  4) ../venv/bin/python permuter.py nonmatchings/<Ad> --stop-on-zero -j 4"
echo
echo "UYARI -- ARA SKORLARA GUVENME. Permuter'in skoru KOMUT AGIRLIKLI bir"
echo "sezgiseldir, bizim olcutumuz (birebir bayt esitligi) DEGIL. Olculdu:"
echo "CleanupAreaTiles'ta skor 50 -> 45 'yeni en iyi' dedi, ama adayin bayt"
echo "farki 7'de KALDI; yalnizca farkin yeri kaydi (0x1a -> 0x1c)."
echo "Anlamli olan TEK deger skor 0'dir; o birebir eslesme demektir."
echo "Her aday, kabul edilmeden once kendi olcumumuzle dogrulanmali:"
echo "  make c-match FILE=<kaynak.c>"
