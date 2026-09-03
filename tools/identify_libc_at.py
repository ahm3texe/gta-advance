#!/usr/bin/env python3
"""Verilen ROM adresinin libc.a'daki hangi fonksiyon oldugunu bulur.

scan_libc.py kor arama yapar: govdeyi ROM'un TAMAMINDA arar. Yer
degistirmenin dokundugu byte'lar joker sayildigi icin, cok fazla
maskelenen kisa fonksiyonlar (orn. newlib'in `_xxx_r` reentrant
sarmalayicilari) o aramada ya bulunmaz ya da ayirt edilemez.

Bu arac tersini yapar: adres BILINIYOR, soru hangi fonksiyon oldugu.
Tek bir noktada karsilastirdigi icin maskelenmis govdeler de ayirt
edilebilir hale gelir.

Kullanim:
    python3 tools/identify_libc_at.py 0x08071D20 [0x08071D5C ...]
"""
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from scan_libc import relocation_offsets  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
LIBC = ROOT / "tools/agbcc/lib/libc.a"
ROM_BASE = 0x08000000
# Zayif eslesmeler yanlis pozitif uretiyor: 12 baytlik bir govdenin 8 sabit
# bayti, ayni bicimdeki HER fonksiyona uyar. Bu depoda 0x08071D50 boyle
# yanlislikla `__errno` sanildi; havuz degerinin bir ROM adresi oldugu
# gorulunce elendi. Bu yuzden hem mutlak hem oransal alt sinir var.
MIN_FIXED = 12
MIN_FIXED_RATIO = 0.6


def text_symbols(obj: Path) -> list[tuple[str, int, int]]:
    """(ad, .text icindeki ofset, boyut) — yalnizca .text'teki fonksiyonlar."""
    out = subprocess.run(["arm-none-eabi-nm", "-S", "--defined-only", str(obj)],
                         capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in ("T", "t"):
            syms.append((parts[3], int(parts[0], 16), int(parts[1], 16)))
    return syms


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    targets = [int(a, 16) for a in sys.argv[1:]]
    rom = ROM.read_bytes()

    work = Path(tempfile.mkdtemp())
    members = subprocess.run(["arm-none-eabi-ar", "t", str(LIBC)],
                             capture_output=True, text=True).stdout.split()
    subprocess.run(["arm-none-eabi-ar", "x", str(LIBC)], cwd=work, check=True)

    candidates = []
    for name in members:
        obj = work / name
        if not obj.exists():
            continue
        text = work / f"{name}.text"
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
                        "--only-section=.text", str(obj), str(text)],
                       capture_output=True)
        if not text.exists() or text.stat().st_size == 0:
            continue
        blob = text.read_bytes()
        spans = relocation_offsets(obj)
        for sym, offset, size in text_symbols(obj):
            if size == 0 or offset + size > len(blob):
                continue
            body = blob[offset:offset + size]
            mask = bytearray(b"\xff" * size)
            for rel, length in spans:
                for i in range(rel - offset, rel - offset + length):
                    if 0 <= i < size:
                        mask[i] = 0
            fixed = [i for i, m in enumerate(mask) if m]
            candidates.append((name, sym, body, fixed))

    print(f"libc.a: {len(candidates)} fonksiyon govdesi hazirlandi\n")
    for address in targets:
        off = address - ROM_BASE
        hits = []
        for name, sym, body, fixed in candidates:
            if len(fixed) < MIN_FIXED or len(fixed) < MIN_FIXED_RATIO * len(body):
                continue
            if off + len(body) > len(rom):
                continue
            chunk = rom[off:off + len(body)]
            if all(chunk[i] == body[i] for i in fixed):
                hits.append((sym, name, len(body), len(fixed)))
        print(f"0x{address:08X}:")
        if not hits:
            print("  eslesme yok")
        for sym, name, size, nfixed in sorted(hits, key=lambda h: -h[3]):
            print(f"  {sym:24s} {name:20s} {size:4d}B  "
                  f"{nfixed}/{size} sabit byte "
                  f"({100 * nfixed // size}%)")


if __name__ == "__main__":
    main()
