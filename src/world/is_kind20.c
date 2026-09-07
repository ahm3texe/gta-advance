/* Tur 20 sinamasi (atlama tablosu) — 0x08067404-0x080674AD
 *
 * Yalnizca tur 20 icin 1 doner; 0..35 arasindaki diger turler ve 35
 * ustu 0. Kaynak bunu `kind == 20` diye DEGIL, 36 girisli bir switch
 * olarak yazmis: ROM'da `cmp #35 / bhi` sonra 36 kelimelik ATLAMA
 * TABLOSU var, 35 girisi `return 0` blogunu gosteriyor.
 *
 * OLCULEN: tabloyu ancak her degerin KENDI `case` etiketi uretiyor;
 * GNU aralik yazimi (`case 0 ... 19:`) iki karsilastirmaya kokup tabloyu
 * kaldiriyor. Etiketlerin gruplu ya da tek tek, case 20'nin once ya da
 * sonra yazilmasi farketmiyor (bes yazim da birebir).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_kind20.c
 */

#include "gba_types.h"

#define KIND_SPECIAL 20

/* 0x08067404 */
u32 IsKind20(u32 kind)
{
    switch (kind) {
    case KIND_SPECIAL:
        return 1;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:        return 0;
    default:
        return 0;
    }
}
