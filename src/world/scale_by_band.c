/* Mesafe bandina gore olcekleme — 0x0806549C-0x08065517
 *
 * Anahtari sekiz banda ayirip her banda 8.8 sabit noktali bir katsayi
 * veriyor (256 = 1.0): 281, 256, 230, 204, 179, 153, 128, 102 — yani
 * yaklasik 1.1'den 0.4'e duzgun dusen bir zayiflama egrisi.
 *
 * Son iki band ROM'da TERS sirali: 102 once yukleniyor, sinir asilmazsa
 * 128 uzerine yaziliyor. Bu, `else if (key <= 0x4FFFF) 128 else 102`
 * zincirinin dogal ciktisi.
 *
 * Karsilastirmalar isaretsiz (ROM `bhi`), carpim sonucu isaretli
 * kaydiriliyor (`asrs`), yani anahtar u32 deger s32.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/scale_by_band.c
 */

#include "gba_types.h"

#define FIXED_SHIFT 8

/* 0x0806549C */
s32 ScaleByDistanceBand(s32 value, u32 key)
{
    s32 factor;

    if (key <= 0x6FFF)
        factor = 281;
    else if (key <= 0x7FFF)
        factor = 256;
    else if (key <= 0xFFFF)
        factor = 230;
    else if (key <= 0x14FFF)
        factor = 204;
    else if (key <= 0x24FFF)
        factor = 179;
    else if (key <= 0x34FFF)
        factor = 153;
    else if (key <= 0x4FFFF)
        factor = 128;
    else
        factor = 102;

    return (value * factor) >> FIXED_SHIFT;
}
