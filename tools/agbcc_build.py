#!/usr/bin/env python3
"""C kaynagini agbcc ile derleyip ROM adresine linkleyen ortak katman.

Linkleme sart: `bl` komutlari hedef adres cozulmeden dogru byte uretmez, bu
yuzden yaprak olmayan hicbir fonksiyon linklenmeden dogrulanamaz. Dis semboller
data/functions.csv'deki ROM adreslerinden cozulur.
"""
import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AGBCC_DIR = ROOT / "tools/agbcc/bin"
ROM_PATH = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
RAM_MAP = ROOT / "data/ram_map.csv"
BUILD = ROOT / "build/cmatch"

DEFAULT_CC = "old_agbcc"
CC1FLAGS = ["-mthumb-interwork", "-O2", "-fhex-asm"]

# ARM kipi.  ROM'un 0x0806xxxx bolgesindeki 18 fonksiyon ARM kipinde ve
# bunlar simdiye kadar DERLENEMIYORDU: zincir yalnizca Thumb'a bagliydi.
# Araç zincirinde `agbcc_arm` bastan beri duruyordu.
# Kaynak dosyada ARM_MARKER satiri varsa bu yol kullanilir.
ARM_MARKER = "KIP: ARM"
ARM_CC = "agbcc_arm"
# `-fhex-asm` agbcc_arm tarafindan TANINMIYOR (olculdu); digerleri kabul.
# -fomit-frame-pointer OLCULDU: onsuz agbcc_arm APCS cercevesi kuruyor
# (mov ip,sp / stmfd {fp,ip,lr,pc} / sub fp,ip,#4) ve cikti ~48 bayt
# uzuyor; ROM duz push kullaniyor.
ARM_CC1FLAGS = ["-mthumb-interwork", "-O2", "-fomit-frame-pointer"]
ROM_BASE = 0x08000000


def run(cmd: list[str], stdout: Path | None = None) -> str:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"HATA: {' '.join(cmd)}\n{result.stderr.strip()}")
    if stdout is not None:
        stdout.write_text(result.stdout, encoding="utf-8")
    return result.stdout


def function_rows() -> dict[str, dict[str, str]]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        return {row["name"]: row for row in csv.DictReader(handle)}


def ram_rows() -> dict[str, dict[str, str]]:
    """RAM sembolleri: kod adresi olmayan dis semboller buradan cozulur."""
    if not RAM_MAP.exists():
        return {}
    with RAM_MAP.open(newline="", encoding="utf-8") as handle:
        return {row["name"]: row for row in csv.DictReader(handle)}


def _symbols(obj: Path) -> dict[str, tuple[int, int]]:
    out = run(["arm-none-eabi-nm", "-S", str(obj)])
    return {
        parts[3]: (int(parts[0], 16), int(parts[1], 16))
        for parts in (line.split() for line in out.splitlines())
        if len(parts) == 4 and parts[2] in "tT"
    }


def _undefined(obj: Path) -> list[str]:
    out = run(["arm-none-eabi-nm", "-u", str(obj)])
    return [
        parts[-1]
        for parts in (line.split() for line in out.splitlines())
        if parts and parts[0] == "U"
    ]


def compile_and_link(source: Path, compiler: str = DEFAULT_CC):
    """C dosyasini derler, ROM adresine linkler ve sonuclari dondurur.

    Donen deger: (linklenmis ikili, {fonksiyon: (ofset, boyut)}, taban adres)
    Ofsetler ikilinin basina goredir.
    """
    agbcc = AGBCC_DIR / compiler
    if not agbcc.exists():
        sys.exit(f"{compiler} kurulu degil. Once: make agbcc")
    if not ROM_PATH.exists():
        sys.exit("baserom.gba yok. Once: make prepare-rom ROM_ZIP=...")

    BUILD.mkdir(parents=True, exist_ok=True)
    stem = BUILD / source.stem

    is_arm = ARM_MARKER in source.read_text(encoding="utf-8")
    if is_arm:
        agbcc = AGBCC_DIR / ARM_CC
        if not agbcc.exists():
            sys.exit(f"{ARM_CC} kurulu degil. Once: make agbcc")
        flags = ARM_CC1FLAGS
    else:
        flags = CC1FLAGS

    run(["cpp", "-nostdinc", "-undef", f"-I{ROOT / 'include'}", str(source)],
        Path(f"{stem}.i"))
    run([str(agbcc), *flags, "-o", f"{stem}.s", f"{stem}.i"])
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.probe.o", f"{stem}.s"])

    rows = function_rows()
    # Dis semboller assembler'a .equ ile verilir: `bl` boylece dogrudan
    # kodlanir ve linker'a hic gitmez. Linker'a birakilirsa, mutlak sembolu
    # Thumb fonksiyonu olarak tanimadigi icin araya interworking veneer'i
    # sokar ve `bl` hedefi yanlis cikar.
    ram = ram_rows()
    externs = []
    for name in _undefined(Path(f"{stem}.probe.o")):
        # `__thumb` soneki: sembol adresi | 1 olarak cozumlenir. Saklanan
        # fonksiyon isaretcilerinde Thumb biti kurulu olmali; `bl` hedefinde
        # ise bit eklemek dal ofsetini bozar, o yuzden AYRI bir ad kullanilir.
        thumb = name.endswith("__thumb")
        key = name[: -len("__thumb")] if thumb else name
        row = rows.get(key) or ram.get(key)
        if row is None:
            sys.exit(f"'{name}' data/functions.csv veya data/ram_map.csv'de yok; "
                     f"adresi cozulemiyor")
        value = int(row["address"], 16) | (1 if thumb else 0)
        externs.append(f"    .equ {name}, {value:#x}\n")
    source_text = Path(f"{stem}.s").read_text(encoding="utf-8")
    # Bolum sonu dolgusu: `as` Thumb bolumlerini varsayilan olarak NOP (0x46C0)
    # ile doldurur, ROM ise sifirla dolduruyor. Acik hizalama bunu duzeltir.
    # ARM komutlari 4 bayt; Thumb 2.  Yanlis hizalama bolum sonunda
    # fazladan dolgu birakip boyutu kaydiriyor.
    align = "4" if is_arm else "2"
    Path(f"{stem}.s").write_text(
        "".join(externs) + source_text + f"\n    .align {align}, 0\n",
        encoding="utf-8",
    )
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.o", f"{stem}.s"])

    obj = Path(f"{stem}.o")
    defined = _symbols(obj)

    known = [rows[name] for name in defined if name in rows]
    if not known:
        sys.exit(f"{source} icindeki hicbir fonksiyon data/functions.csv'de yok")
    base = min(int(row["address"], 16) for row in known)

    # Bolum adresi acikca verilir: agbcc .text'i 8'e hizaliyor ve taban adres
    # 8'in kati degilse linker bolumu ileri iterek tum olcumleri kaydiriyor.
    script = Path(f"{stem}.ld")
    script.write_text(
        "SECTIONS\n{\n"
        f"    . = {base:#x};\n"
        f"    .text {base:#x} : SUBALIGN(1) {{ *(.text) }}\n"
        + "    /DISCARD/ : { *(.comment) *(.note*) }\n}\n",
        encoding="utf-8",
    )
    run(["arm-none-eabi-ld", "-T", str(script), "-o", f"{stem}.elf", f"{stem}.o"])
    run(["arm-none-eabi-objcopy", "-O", "binary", "--only-section=.text",
         f"{stem}.elf", f"{stem}.bin"])

    linked = _symbols(Path(f"{stem}.elf"))
    layout = {
        name: (address - base, size)
        for name, (address, size) in linked.items()
        if name in rows
    }
    return Path(f"{stem}.bin").read_bytes(), layout, base


def rom_bytes() -> bytes:
    return ROM_PATH.read_bytes()
