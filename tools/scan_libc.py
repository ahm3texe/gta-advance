#!/usr/bin/env python3
"""agbcc'nin libc.a'sindaki fonksiyonlari ROM icinde arar.

ROM, agbcc ile birlikte gelen newlib'e linkleniyor: "bug in vfprintf: bad base",
"_sbrk: Heap and stack collision" ve "Infinity" dizeleri hem ROM'da hem
tools/agbcc/lib/libc.a icinde birebir var. Yani kod bolgesinin kuyrugu tersine
muhendislik gerektirmeyen standart kutuphane kodudur; kaynagi zaten elimizde.

Dis cagri iceren fonksiyonlar linklenmeden ROM byte'lariyla eslesmez: `bl`
hedefi ve literal havuzdaki adresler baglama gore degisir. Bu yuzden tarama
MASKELI yapilir - yer degistirmenin dokundugu byte'lar joker sayilir, geri
kalan govde birebir aranir. Boylece adresi bilmeden de fonksiyon bulunur.

Kullanim:  python3 tools/scan_libc.py [--csv]
"""
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIBC = ROOT / "tools/agbcc/lib/libc.a"
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
ROM_BASE = 0x08000000
MIN_SIZE = 16       # kisa govdeler tesaduf eslesme uretir
MIN_FIXED = 12      # maskeleme sonrasi en az bu kadar sabit byte kalmali


def relocation_offsets(obj: Path) -> list[tuple[int, int]]:
    """(.text icindeki ofset, uzunluk) ciftleri."""
    out = subprocess.run(["arm-none-eabi-objdump", "-r", str(obj)],
                         capture_output=True, text=True).stdout
    spans, in_text = [], False
    for line in out.splitlines():
        if line.startswith("RELOCATION RECORDS FOR"):
            in_text = "[.text]" in line
            continue
        if not in_text or not line or line.startswith("OFFSET"):
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        try:
            offset = int(parts[0], 16)
        except ValueError:
            continue
        # Thumb BL 4 byte, ABS32 literal 4 byte; hepsini 4 byte kabul et.
        spans.append((offset, 4))
    return spans


def masked_find(rom: bytes, body: bytes, spans: list[tuple[int, int]],
                start: int) -> int:
    """Joker byte'lar iceren govdeyi ROM'da arar; bulunan mutlak ofseti doner."""
    mask = bytearray(b"\xff" * len(body))
    for offset, length in spans:
        for i in range(offset - start, offset - start + length):
            if 0 <= i < len(mask):
                mask[i] = 0
    fixed = [i for i, m in enumerate(mask) if m]
    if len(fixed) < MIN_FIXED:
        return -1
    # En uzun sabit parcayi kaba filtre olarak kullan.
    runs, run = [], []
    for i in fixed:
        if run and i == run[-1] + 1:
            run.append(i)
        else:
            if run:
                runs.append(run)
            run = [i]
    if run:
        runs.append(run)
    anchor = max(runs, key=len)
    needle = body[anchor[0]:anchor[-1] + 1]
    if len(needle) < 4:
        return -1
    position = rom.find(needle)
    while position >= 0:
        base = position - anchor[0]
        if base >= 0 and base + len(body) <= len(rom):
            candidate = rom[base:base + len(body)]
            if all(candidate[i] == body[i] for i in fixed):
                return base
        position = rom.find(needle, position + 1)
    return -1


def main() -> None:
    if not LIBC.exists():
        sys.exit("tools/agbcc/lib/libc.a yok. Once: make agbcc")
    if not ROM.exists():
        sys.exit("baserom.gba yok.")

    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        known = {int(r["address"], 16) for r in csv.DictReader(handle)}

    work = Path(tempfile.mkdtemp())
    subprocess.run(["arm-none-eabi-ar", "x", str(LIBC)], cwd=work, capture_output=True)
    binary = work / "slice.bin"

    found, checked, masked = [], 0, 0
    for obj in sorted(work.glob("*.o")):
        nm = subprocess.run(["arm-none-eabi-nm", "-S", str(obj)],
                            capture_output=True, text=True).stdout
        symbols = [
            (p[3], int(p[0], 16), int(p[1], 16))
            for p in (line.split() for line in nm.splitlines())
            if len(p) == 4 and p[2] in "tT"
        ]
        if not symbols:
            continue
        spans = relocation_offsets(obj)
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(binary)],
                       capture_output=True)
        text = binary.read_bytes()
        for name, offset, size in symbols:
            if size < MIN_SIZE or offset + size > len(text):
                continue
            checked += 1
            body = text[offset:offset + size]
            local = [(o, l) for o, l in spans if offset <= o < offset + size]
            if local:
                index = masked_find(rom, body, local, offset)
                masked += 1
            else:
                index = rom.find(body)
            if index >= 0:
                found.append((ROM_BASE + index, name, size, bool(local)))

    # Ayni govdeye sahip semboller (ornegin toupper/_toupper) ayni adresi bulur.
    # Hangisinin o adreste durdugu byte'lardan anlasilamaz; uydurmak yerine
    # belirsiz olarak isaretlenir.
    grouped: dict[int, list] = {}
    for address, name, size, was_masked in found:
        grouped.setdefault(address, []).append((name, size, was_masked))
    found = []
    for address in sorted(grouped):
        entries = grouped[address]
        names = sorted(n for n, _, _ in entries)
        _, size, was_masked = entries[0]
        found.append((address, names[0], size, was_masked, names[1:]))

    if "--csv" in sys.argv:
        print("address,name,size,status,module,notes")
        for address, name, size, was_masked, aliases in found:
            note = ("newlib routine; matched against agbcc libc.a build"
                    if was_masked else
                    "newlib routine; body byte-identical to agbcc libc.a build")
            if aliases:
                note += (f"; identical body to {', '.join(aliases)} - which symbol "
                         f"occupies this address is undetermined")
            if address not in known:
                note += "; missed by Ghidra auto-analysis"
            status = "discovered" if aliases else "documented"
            print(f'0x{address:08X},{name},{size},{status},libc,"{note}"')
        return

    print(f"libc.a: {checked} fonksiyon arandi ({masked} tanesi maskeli)")
    print(f"ROM'da bulunan: {len(found)}\n")
    for address, name, size, was_masked, aliases in found:
        flags = []
        if aliases:
            flags.append(f"belirsiz: {'/'.join([name] + aliases)}")
        if was_masked:
            flags.append("maskeli")
        if address not in known:
            flags.append("Ghidra kacirmis")
        tail = f"   <- {', '.join(flags)}" if flags else ""
        print(f"  0x{address:08X}  {name:20} {size:>4}B{tail}")


if __name__ == "__main__":
    main()
