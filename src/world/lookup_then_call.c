/* Look up a record word and forward — 0x08030C0C-0x08030C27
 *
 * Preserve both arguments ACROSS THE CALL and pass them with GetTextString's
 * result to FUN_0802DF18. The ROM copies them to r4/r5 (rule 37: values that
 * must survive a call need separate locals).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/lookup_then_call.c
 */

#include "gba_types.h"

extern u32  GetTextString(u32 index);
extern void FUN_0802df18(u32 word, u32 b, u32 c);

/* 0x08030C0C */
void LookupThenCall(u32 index, u32 b, u32 c)
{
    FUN_0802df18(GetTextString(index), b, c);
}
