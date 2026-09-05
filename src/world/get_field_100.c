/* +0x100 alanini okuma — 0x0801D8B0-0x0801D8BB
 *
 * ROM ofseti `0x80 << 1` ile kuruyor (movs #128 + lsls #1); 256 ani degere
 * sigmadigi icin kaydirmali bicim gerekiyor. Duz 256 yazmak havuz
 * yuklemesi uretirdi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_field_100.c
 */

#include "gba_types.h"

/* 0x0801D8B0 */
u32 GetField100(u8 *base)
{
    return *(u32 *)(base + (0x80 << 1));
}
