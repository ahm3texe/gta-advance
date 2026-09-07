/* Iletme sarmalayicisi — 0x08059D7C-0x08059D85
 *
 * Govdesi yalnizca GetAnchorUnk30 cagrisi. tools/find_wrappers.py ile bulundu;
 * ilk taramada hedefi haritada olmadigi icin atlanmisti -- hedef bu tur
 * sayesinde KESFEDILDI ve haritaya eklendi.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 DONUS DEGERI tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/wrap_08059d7c.c
 */

#include "gba_types.h"

extern u32 GetAnchorUnk30(void);

/* 0x08059D7C */
u32 ForwardToGetAnchorUnk30(void)
{
    return GetAnchorUnk30();
}
