#!/usr/bin/env python3
"""Permuter icin tek bir C kaynagini .o'ya derler.

Neden ayri arac: permuter'in bekledigi "cc -c girdi.c -o cikti.o" bicimi bizim
boru hattimizi TAM olarak temsil etmiyor. Bizde dis semboller `.equ` ile
mutlak adrese cevriliyor (agbcc_build.py'deki gerekce: linker aksi halde
interworking veneer'i sokup `bl` hedefini bozuyor). Permuter mutasyonlu
kaynaklari gecici dizinlerde derledigi icin bu adim orada da calismali.

Kullanim: python3 tools/permuter_compile.py [-o <cikti.o>] <girdi.c>

`-o` bayragi ZORUNLU ama SIRASI serbest. Permuter, derleme komutunu bir
DERLEYICI cagrisi olarak tanimak icin `-o` ariyor (bulamazsa adayi eliyor);
uretttigi compile.sh ise betigi `<girdi> -o <cikti>` sirasiyla cagiriyor.
Iki sira da desteklenmeli.
"""
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import (  # noqa: E402
    AGBCC_DIR, CC1FLAGS, DEFAULT_CC, ROOT, _undefined, function_rows, ram_rows,
)


def run(cmd, out=None):
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"{cmd[0]} basarisiz:\n{result.stderr.strip()[:800]}")
    if out is not None:
        out.write_text(result.stdout, encoding="utf-8")


def main() -> None:
    args = sys.argv[1:]
    target_arg = None
    positional = []
    i = 0
    while i < len(args):
        if args[i] == "-o" and i + 1 < len(args):
            target_arg = args[i + 1]
            i += 2
            continue
        positional.append(args[i])
        i += 1
    if target_arg is None or len(positional) != 1:
        sys.exit(__doc__)
    target, source = Path(target_arg).resolve(), Path(positional[0]).resolve()
    target.parent.mkdir(parents=True, exist_ok=True)
    work = target.parent
    stem = work / source.stem
    agbcc = AGBCC_DIR / DEFAULT_CC

    run(["cpp", "-nostdinc", "-undef", f"-I{ROOT / 'include'}", str(source)],
        Path(f"{stem}.i"))
    run([str(agbcc), *CC1FLAGS, "-o", f"{stem}.s", f"{stem}.i"])
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.probe.o", f"{stem}.s"])

    rows, ram = function_rows(), ram_rows()
    externs = []
    for name in _undefined(Path(f"{stem}.probe.o")):
        row = rows.get(name) or ram.get(name)
        if row is None:
            sys.exit(f"'{name}' data/functions.csv veya data/ram_map.csv'de yok")
        externs.append(f"    .equ {name}, {int(row['address'], 16):#x}\n")
    text = Path(f"{stem}.s").read_text(encoding="utf-8")
    Path(f"{stem}.s").write_text(
        "".join(externs) + text + "\n    .align 2, 0\n", encoding="utf-8")

    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.o", f"{stem}.s"])
    if Path(f"{stem}.o") != target:
        shutil.copy(Path(f"{stem}.o"), target)


if __name__ == "__main__":
    main()
