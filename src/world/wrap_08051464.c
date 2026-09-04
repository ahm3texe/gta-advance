/* Iletme sarmalayicisi — 0x08051464-0x0805146D
 *
 * Govdesi yalnizca FUN_080504b4 cagrisi. Ne yaptigi bilinmedigi icin ad
 * degistirilmedi. tools/find_wrappers.py ile bulundu.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_08051464.c
 */

#include "gba_types.h"

extern void FUN_080504b4(void);

/* 0x08051464 */
void FUN_08051464(void)
{
    FUN_080504b4();
}
