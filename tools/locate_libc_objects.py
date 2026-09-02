#!/usr/bin/env python3
"""libc.a nesnelerini bilinen bir capadan hizalayip fonksiyon fonksiyon karsilastirir.

AMACI TANI KOYMAKTIR, eslestirme degil. Nesne duzeyinde yerlestirme denendi ve
CALISMADI: ROM'un newlib'i ayni kaynaktan geliyor ama farkli yapilandirmayla
derlenmis. Ornegin syscalls.o'da `findslot` ve `remap_handle` birebir tutuyor,
`initialise_monitor_handles` kismen tutuyor, ondan sonraki her sey kayiyor -
cunku sistem cagrisi saplamalari oyuna ozel yazilmis ve boyutlari farkli.

Bu arac o ayrismayi gosterir: bir nesnenin hangi kismi ortak, hangi kismi
oyuna ozel. Gercek eslestirme icin tools/scan_libc.py (fonksiyon duzeyinde
maskeli arama) kullanilir.

Kullanim:  python3 tools/locate_libc_objects.py <capa_fonksiyonu> <rom_adresi>
Ornek:     python3 tools/locate_libc_objects.py remap_handle 0x0807180C
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
MIN_ANCHOR = 24      # capa olarak kullanilacak en kucuk sabit dizi
MIN_VERIFY = 64      # nesne dogrulamasi icin gereken en az sabit byte


def run(cmd: list[str]) -> str:
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def text_bytes(obj: Path, out: Path) -> bytes:
    subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                    "--only-section=.text", str(obj), str(out)],
                   capture_output=True)
    return out.read_bytes() if out.exists() else b""


def symbols(obj: Path) -> list[tuple[str, int, int]]:
    return [
        (p[3], int(p[0], 16), int(p[1], 16))
        for p in (line.split() for line in run(["arm-none-eabi-nm", "-S", str(obj)]).splitlines())
        if len(p) == 4 and p[2] in "tT"
    ]


def reloc_offsets(obj: Path) -> list[int]:
    out, in_text, spans = run(["arm-none-eabi-objdump", "-r", str(obj)]), False, []
    for line in out.splitlines():
        if line.startswith("RELOCATION RECORDS FOR"):
            in_text = "[.text]" in line
            continue
        if not in_text or not line or line.startswith("OFFSET"):
            continue
        parts = line.split()
        try:
            spans.append(int(parts[0], 16))
        except (ValueError, IndexError):
            continue
    return spans


def build_mask(length: int, relocs: list[int]) -> bytearray:
    mask = bytearray(b"\xff" * length)
    for offset in relocs:
        for i in range(offset, min(offset + 4, length)):
            mask[i] = 0
    return mask


def runs_of(mask: bytearray) -> list[tuple[int, int]]:
    """Maskedeki kesintisiz sabit bolgeler: (baslangic, uzunluk)."""
    out, start = [], None
    for i, m in enumerate(mask):
        if m and start is None:
            start = i
        elif not m and start is not None:
            out.append((start, i - start))
            start = None
    if start is not None:
        out.append((start, len(mask) - start))
    return out


def locate(rom: bytes, text: bytes, mask: bytearray) -> int | None:
    """Nesnenin ROM'daki taban ofsetini bulur; belirsizse None."""
    spans = [r for r in runs_of(mask) if r[1] >= MIN_ANCHOR]
    if not spans or sum(l for _, l in runs_of(mask)) < MIN_VERIFY:
        return None
    anchor_off, anchor_len = max(spans, key=lambda r: r[1])
    needle = text[anchor_off:anchor_off + anchor_len]
    hits, position = [], rom.find(needle)
    while position >= 0:
        base = position - anchor_off
        if base >= 0 and base + len(text) <= len(rom):
            if all(rom[base + i] == text[i] for i in range(len(text)) if mask[i]):
                hits.append(base)
        position = rom.find(needle, position + 1)
    return hits[0] if len(hits) == 1 else None


def decode_thumb_bl(rom: bytes, offset: int) -> int | None:
    """ROM'daki Thumb BL'i cozup hedef adresi dondurur."""
    if offset + 4 > len(rom):
        return None
    h1 = rom[offset] | (rom[offset + 1] << 8)
    h2 = rom[offset + 2] | (rom[offset + 3] << 8)
    if (h1 & 0xF800) != 0xF000 or (h2 & 0xF800) != 0xF800:
        return None
    value = ((h1 & 0x7FF) << 12) | ((h2 & 0x7FF) << 1)
    if value & (1 << 22):
        value -= 1 << 23
    return ROM_BASE + offset + 4 + value


def main() -> None:
    if not LIBC.exists():
        sys.exit("tools/agbcc/lib/libc.a yok. Once: make agbcc")
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    anchor_name, anchor_address = sys.argv[1], int(sys.argv[2], 16)

    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        known = {int(r["address"], 16): r["name"] for r in csv.DictReader(handle)}

    work = Path(tempfile.mkdtemp())
    subprocess.run(["arm-none-eabi-ar", "x", str(LIBC)], cwd=work, capture_output=True)
    slice_path = work / "slice.bin"

    for obj in sorted(work.glob("*.o")):
        syms = symbols(obj)
        match = next((s for s in syms if s[0] == anchor_name), None)
        if match is None:
            continue

        text = text_bytes(obj, slice_path)
        mask = build_mask(len(text), reloc_offsets(obj))
        base = (anchor_address - ROM_BASE) - match[1]

        print(f"{obj.name}  ({len(text)} byte)  capa: {anchor_name} @ "
              f"0x{anchor_address:08X}  ->  taban 0x{ROM_BASE + base:08X}\n")
        print(f"{'fonksiyon':28} {'ROM adresi':>12} {'boyut':>6}  sabit byte uyumu")
        print("-" * 74)
        exact = 0
        for name, offset, size in sorted(syms, key=lambda s: s[1]):
            if not size:
                continue
            fixed = [i for i in range(offset, min(offset + size, len(text))) if mask[i]]
            if not fixed:
                continue
            agree = sum(1 for i in fixed if rom[base + i] == text[i])
            verdict = "TAM" if agree == len(fixed) else ""
            if verdict:
                exact += 1
            flag = "" if ROM_BASE + base + offset in known else " (haritada yok)"
            print(f"{name:28} 0x{ROM_BASE + base + offset:08X} {size:>6}  "
                  f"{agree}/{len(fixed)} {verdict}{flag}")
        print("-" * 74)
        print(f"{exact} fonksiyon bu hizalamada birebir tutuyor.")
        print("Kayma basladiktan sonraki satirlar anlamsizdir: bir fonksiyonun "
              "boyutu farkliysa sonraki her sey oteler.")
        return

    sys.exit(f"'{anchor_name}' libc.a icinde bulunamadi")


if __name__ == "__main__":
    main()
