/* Iletme sarmalayicisi — 0x0804FF98-0x0804FFA1
 *
 * Govdesi yalnizca FUN_080457f8 cagrisi. Ne yaptigi bilinmedigi icin ad
 * degistirilmedi. tools/find_wrappers.py ile bulundu.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_0804ff98.c
 */

#include "gba_types.h"

extern u32 FUN_080457f8(void);

/* 0x0804FF98 */
u32 FUN_0804ff98(void)
{
    return FUN_080457f8();
}
