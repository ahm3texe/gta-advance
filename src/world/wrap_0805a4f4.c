/* Iletme sarmalayicisi — 0x0805A4F4-0x0805A4FD
 *
 * Govdesi yalnizca HalvesEqual cagrisi. Ne yaptigi bilinmedigi icin ad
 * degistirilmedi. tools/find_wrappers.py ile bulundu.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_0805a4f4.c
 */

#include "gba_types.h"

extern u32 HalvesEqual(void);

/* 0x0805A4F4 */
u32 ForwardToHalvesEqual(void)
{
    return HalvesEqual();
}
