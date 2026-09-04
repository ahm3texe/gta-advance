/* Iletme sarmalayicisi — 0x080500D0-0x080500D9
 *
 * Govdesi yalnizca FUN_0804bfbc cagrisi. Ne yaptigi bilinmedigi icin ad
 * degistirilmedi. tools/find_wrappers.py ile bulundu.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_080500d0.c
 */

#include "gba_types.h"

extern u32 FUN_0804bfbc(void);

/* 0x080500D0 */
u32 FUN_080500d0(void)
{
    return FUN_0804bfbc();
}
