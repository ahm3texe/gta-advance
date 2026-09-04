/* Iletme sarmalayicisi — 0x0804FFB4-0x0804FFBD
 *
 * Govdesi yalnizca FUN_08046358 cagrisi. Ne yaptigi bilinmedigi icin ad
 * degistirilmedi. tools/find_wrappers.py ile bulundu.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_0804ffb4.c
 */

#include "gba_types.h"

extern u32 FUN_08046358(void);

/* 0x0804FFB4 */
u32 FUN_0804ffb4(void)
{
    return FUN_08046358();
}
