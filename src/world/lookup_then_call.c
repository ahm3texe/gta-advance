/* Kayit sozunu bulup iletme — 0x08030C0C-0x08030C27
 *
 * Iki argumani CAGRI BOYUNCA saklayip GetRecordWord sonucuyla birlikte
 * FUN_0802DF18'e veriyor. ROM ikisini de r4/r5'e kopyaliyor (kural 37:
 * cagri boyunca yasamasi gereken degerler ayri yerellerde).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/lookup_then_call.c
 */

#include "gba_types.h"

extern u32  GetRecordWord(u32 index);
extern void FUN_0802df18(u32 word, u32 b, u32 c);

/* 0x08030C0C */
void LookupThenCall(u32 index, u32 b, u32 c)
{
    FUN_0802df18(GetRecordWord(index), b, c);
}
