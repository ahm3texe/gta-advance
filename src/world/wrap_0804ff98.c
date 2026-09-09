/* Forwarding wrapper — 0x0804FF98-0x0804FFA1
 *
 * The body only calls FUN_080457f8. Its purpose is unknown, so its name
 * is unchanged. Found with tools/find_wrappers.py.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_0804ff98.c
 */

#include "gba_types.h"

extern u32 FUN_080457f8(void);

/* 0x0804FF98 */
u32 FUN_0804ff98(void)
{
    return FUN_080457f8();
}
