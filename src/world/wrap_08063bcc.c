/* Iletme sarmalayicisi — 0x08063BCC-0x08063BD5
 *
 * Govdesi yalnizca FUN_08010224 cagrisi. Ne yaptigi bilinmedigi icin ad
 * degistirilmedi. tools/find_wrappers.py ile bulundu.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_08063bcc.c
 */

#include "gba_types.h"

extern void FUN_08010224(void);

/* 0x08063BCC */
void FUN_08063bcc(void)
{
    FUN_08010224();
}
