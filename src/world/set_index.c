/* Tablo indeksi ayarlama — 0x08008094-0x080080A1
 *
 * Ikinci argumani SetTableIndex'e verip 1 donduruyor; ilk arguman
 * kullanilmiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/set_index.c
 */

#include "gba_types.h"

extern void SetTableIndex(u32 index);

/* 0x08008094 */
u32 SetIndexReturnOne(u32 unused, u32 index)
{
    SetTableIndex(index);

    return 1;
}
