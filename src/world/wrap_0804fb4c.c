/* Forwarding wrapper — 0x0804FB4C-0x0804FB57
 *
 * The body only calls FUN_080457f8. Its purpose is UNKNOWN, so the
 * FUN_ name is retained rather than inventing unsupported semantics.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE; the return type is u32.
 * Arguments are already in r0-r3, so the wrapper does not touch them;
 * declaring it without arguments would still produce the same bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_0804fb4c.c
 */

#include "gba_types.h"

extern u32 FUN_080457f8(void);

/* 0x0804FB4C */
u32 FUN_0804fb4c(void)
{
    return FUN_080457f8();
}
