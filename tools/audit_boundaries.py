#!/usr/bin/env python3
"""data/functions.csv'deki fonksiyon sinirlarini ROM'a karsi denetler.

Ghidra atlama tablolarinda cozumleyemeyip fonksiyonlari ortadan kesiyor.
Yanlis sinir her asamada bosa emek demek: bir fonksiyonu tek basina
yazmaya calisirken aslinda bir parcasini yaziyor olursun.

YONTEM: ozyinelemeli inis. Giristen baslayip yalnizca GERCEKTEN ULASILAN
komutlar cozumlenir. Dogrusal disassembly kullanilamaz, cunku literal
havuzu ve veri baytlari komut gibi cozulup sahte dallanma uretiyor; bu
depoda o yaklasim 998 fonksiyonu 8 KB'a "buyutmustu".

Literal havuzu adresleri VERI olarak isaretlenir, asla cozumlenmez.
Atlama tablolari `mov pc, rN`den geriye dogru bulunur ve girdiler
yalnizca makul araliktayken izlenir.

Kullanim:
    python3 tools/audit_boundaries.py            # rapor + kendi kendini sinama
    python3 tools/audit_boundaries.py --apply    # functions.csv'yi duzelt
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
FUNCTIONS = ROOT / "data/functions.csv"
REGIONS = ROOT / "data/matching_regions.csv"
ROM_BASE = 0x08000000
MAX_EXTENT = 8192

# Kalici ARM fonksiyonlari: yurutucu Thumb cozumluyor, bunlar atlanir.
ARM_FUNCTIONS = {0x080000C0, 0x08000104}

# ARM kodu bolgesi. Olculdu: TEK bitisik aralik, dort ayri parca DEGIL.
# [0x08067E04, 0x0806B84C) = 14920 bayt. Kanit: aralikta 3730 kelimenin
# TAMAMINDA kosul alani != 0xF (rastgele veri/Thumb'da ~1/16 kelimede
# 0xF beklenir); hemen oncesi 0.9533, sonrasi 0.9747 oraninda kaliyor.
# Ust sinir 0x0806B84C = BIOS swi thunk'larinin (Thumb) basi.
#
# Onceki dort-aralikli sabit yanlis pozitif icermiyordu ama 6524 bayti
# (%44) kaciriyordu; kacirilan dort parcanin dordu de 1.0000 oran veriyor.
#
# Icerik: ses surucusu DEGIL (ROM'da m4a/sappy imzasi ve bu bolgede tek
# bir ses yazmaci erisimi yok) -- afin doku esleme, Cohen-Sutherland
# kirpma ve 8bpp doseli cerceve arabellegi adresleme, yani rasterlestirici.
ARM_RANGES = [
    (0x08067E04, 0x0806B84C),
]

# Bir fonksiyonun bu boyutu asmasi analiz hatasi sayilir; otomatik
# duzeltilmez, elle bakilmak uzere raporlanir.
SANE_MAX = 4096


def in_arm_range(addr: int) -> bool:
    return any(lo <= addr < hi for lo, hi in ARM_RANGES)


class Walker:
    """Tek bir fonksiyonun ulasilabilir komutlarini gezer."""

    def __init__(self, rom: bytes, start: int):
        self.rom = rom
        self.start = start
        self.code = set()       # cozumlenen komut adresleri
        self.data = set()       # literal havuzu kelimeleri
        self.unresolved = False  # cozulemeyen dolayli atlama var mi

    def hw(self, addr: int) -> int:
        off = addr - ROM_BASE
        return int.from_bytes(self.rom[off:off + 2], "little")

    def word(self, addr: int) -> int:
        off = addr - ROM_BASE
        return int.from_bytes(self.rom[off:off + 4], "little")

    def in_range(self, addr: int) -> bool:
        return self.start <= addr < self.start + MAX_EXTENT

    def jump_table(self, dispatch: int) -> list[int]:
        """`mov pc, rN` oncesindeki havuz yuklemesinden tablo girdilerini oku.

        Desen: ldr rX, [pc, #N] -> tablo tabani; lsl/add; ldr; mov pc.
        Tablo, araliktaki cift adresler surdugu surece okunur.
        """
        base = None
        for back in range(2, 20, 2):
            addr = dispatch - back
            if addr < self.start:
                break
            h = self.hw(addr)
            if (h & 0xF800) == 0x4800:                      # ldr rX, [pc, #imm]
                pool = ((addr + 4) & ~3) + ((h & 0xFF) * 4)
                candidate = self.word(pool)
                if self.in_range(candidate):
                    base = candidate
                    self.data.update(range(pool, pool + 4))
                    break
        if base is None:
            return []
        targets = []
        for i in range(256):
            entry = base + 4 * i
            value = self.word(entry)
            if value % 2 or not self.in_range(value) or value < self.start:
                break
            targets.append(value)
            self.data.update(range(entry, entry + 4))
        return targets

    def run(self) -> None:
        pending = [self.start]
        while pending:
            addr = pending.pop()
            while True:
                if addr in self.code or not self.in_range(addr):
                    break
                if addr in self.data:
                    break
                self.code.add(addr)
                h = self.hw(addr)
                nxt = addr + 2

                if (h & 0xF800) == 0xF000:                  # 32 bit bl/blx
                    self.code.add(addr + 2)
                    addr += 4
                    continue
                if (h & 0xF800) == 0x4800:                  # ldr rX, [pc, #imm]
                    pool = ((addr + 4) & ~3) + ((h & 0xFF) * 4)
                    if self.in_range(pool):
                        self.data.update(range(pool, pool + 4))
                    addr = nxt
                    continue
                if (h & 0xF000) == 0xD000:                  # kosullu dal / swi
                    cond = (h >> 8) & 0xF
                    if cond < 0xE:
                        off = h & 0xFF
                        if off & 0x80:
                            off -= 0x100
                        pending.append(addr + 4 + off * 2)
                    addr = nxt
                    continue
                if (h & 0xF800) == 0xE000:                  # kosulsuz dal
                    off = h & 0x7FF
                    if off & 0x400:
                        off -= 0x800
                    pending.append(addr + 4 + off * 2)
                    break
                if h == 0x4770:                             # bx lr
                    break
                if (h & 0xFF00) == 0xBD00:                  # pop {..., pc}
                    break
                if (h & 0xFF87) == 0x4700:                  # bx rN
                    break
                if (h & 0xFF87) == 0x4687:                  # mov pc, rN
                    targets = self.jump_table(addr)
                    if targets:
                        pending.extend(targets)
                    else:
                        self.unresolved = True
                    break
                addr = nxt

    def code_extent(self) -> int:
        """Yalnizca ulasilan KOMUTLARIN kapladigi uzunluk.

        CSV'deki `size` govde uzunlugudur, literal havuzunu icermez; gercek
        sinir hatasi ancak kod govdeyi asinca vardir.
        """
        return (max(self.code) + 2 - self.start) if self.code else 0

    def full_extent(self) -> int:
        """Kod + literal havuzu: matching_regions.csv'nin kullandigi olcu."""
        covered = self.code | self.data
        return (max(covered) + 4 - self.start) if covered else 0


def main() -> None:
    apply = "--apply" in sys.argv
    rom = ROM.read_bytes()
    with FUNCTIONS.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fields = list(rows[0].keys())

    # Kendi kendini sinama. Dogru degismez su: byte-matching bir fonksiyonun
    # ULASILABILIR KODU, onu iceren dogrulanmis bolgenin disina tasamaz.
    # (functions.csv'deki `size` alani bunun icin kullanilamaz: bazi matching
    # fonksiyonlarda o alan bayat, orn. VBlankIntr 312 diyor ama govde
    # satir ici havuzlarla birlikte bolgenin sonuna kadar uzaniyor.)
    regions = [(int(r["start"], 16), int(r["end"], 16))
               for r in csv.DictReader(REGIONS.open(newline="", encoding="utf-8"))]

    def containing(addr: int):
        for lo, hi in regions:
            if lo <= addr < hi:
                return lo, hi
        return None

    print("Kendi kendini sinama (byte-matching fonksiyonlar):")
    bad = checked = 0
    for row in rows:
        if row["status"] != "matching":
            continue
        start = int(row["address"], 16)
        if start in ARM_FUNCTIONS:
            continue
        region = containing(start)
        if region is None:
            continue
        checked += 1
        w = Walker(rom, start)
        w.run()
        if w.code and max(w.code) + 2 > region[1]:
            bad += 1
            if bad <= 5:
                print(f"  TASMA {row['address']} {row['name']}: "
                      f"bolge 0x{region[1]:08X}, ulasilan kod "
                      f"0x{max(w.code) + 2:08X}")
    print(f"  {checked} fonksiyon sinandi, {bad} tasma\n")
    if bad:
        print("Arac dogrulanmis fonksiyonlarda tasiyor; --apply guvenli degil.")
        if apply:
            sys.exit(1)

    known = sorted(int(r["address"], 16) for r in rows)
    verified = {int(r["address"], 16) for r in rows if r["status"] == "matching"}
    grown, swallowed = [], {}
    skipped_arm, skipped_jump, skipped_big, conflicts = [], [], [], []
    for row in rows:
        if not row["size"].strip() or row["status"] == "matching":
            continue
        start, size = int(row["address"], 16), int(row["size"])
        if size <= 0:
            continue
        if start in ARM_FUNCTIONS or in_arm_range(start):
            skipped_arm.append(row)
            continue
        w = Walker(rom, start)
        w.run()
        got = w.code_extent()
        if got <= size:
            continue
        # Muhafazakar kapi: cozulemeyen dolayli atlama varsa yurutucu
        # govdenin bir kismini gormemis olabilir, akil disi buyume ise
        # kip hatasina isaret eder. Ikisi de otomatik duzeltilmez.
        if w.unresolved:
            skipped_jump.append((row, size, got))
            continue
        if got > SANE_MAX:
            skipped_big.append((row, size, got))
            continue
        eaten = [a for a in known if start < a < start + got]
        # Guvenlik kapisi: byte-matching dogrulanmis bir kaydin uzerine
        # buyume kabul edilmez. Boyle bir catisma aracin yanildigina isaret
        # eder; elle bakilmak uzere raporlanir.
        if any(a in verified for a in eaten):
            conflicts.append((row, size, got))
            continue
        grown.append((row, size, got, eaten))
        for a in eaten:
            swallowed[a] = start

    print(f"{len(grown)} fonksiyonun siniri kisa; "
          f"{len(swallowed)} kayit baska bir fonksiyonun icinde kaliyor.")
    print(f"Denetim disi birakilanlar (sessizce atilmadi, elle bakilmali):")
    print(f"  {len(skipped_arm):4d} ARM araliginda")
    print(f"  {len(skipped_jump):4d} cozulemeyen dolayli atlama iceriyor")
    print(f"  {len(skipped_big):4d} akil disi buyume (>{SANE_MAX} bayt), "
          f"kip hatasi olabilir")
    print(f"  {len(conflicts):4d} dogrulanmis bir fonksiyonun uzerine buyuyor")
    print()
    print(f"{'adres':12} {'eski':>6} {'yeni':>6}  yutulan")
    print("-" * 58)
    for row, old, new, eaten in sorted(grown, key=lambda g: g[2] - g[1],
                                       reverse=True)[:20]:
        tag = " ".join(f"0x{a:08X}" for a in eaten[:3])
        if len(eaten) > 3:
            tag += f" (+{len(eaten) - 3})"
        print(f"{row['address']} {old:>6} {new:>6}  {tag}")

    if not apply:
        print("\n(yalnizca rapor; degistirmek icin --apply)")
        return

    out = []
    for row in rows:
        if int(row["address"], 16) in swallowed:
            continue
        for grown_row, _old, new, _eaten in grown:
            if grown_row is row:
                row["size"] = str(new)
                note = row["notes"].strip('"')
                row["notes"] = (note + "; sinir ROM'a karsi duzeltildi "
                                "(audit_boundaries.py)").lstrip("; ")
                break
        out.append(row)

    with FUNCTIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(out)
    print(f"\nfunctions.csv guncellendi: {len(grown)} boyut duzeltildi, "
          f"{len(swallowed)} kayit silindi.")


if __name__ == "__main__":
    main()
