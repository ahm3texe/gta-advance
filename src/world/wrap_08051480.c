/* Iletme sarmalayicisi — 0x08051480-0x08051489
 *
 * Govdesi yalnizca GetAnchorUnk04 cagrisi. tools/find_wrappers.py ile bulundu;
 * ilk taramada hedefi haritada olmadigi icin atlanmisti -- hedef bu tur
 * sayesinde KESFEDILDI ve haritaya eklendi.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_08051480.c
 */

#include "gba_types.h"

extern u32 GetAnchorUnk04(void);

/* 0x08051480 */
u32 FUN_08051480(void)
{
    return GetAnchorUnk04();
}
