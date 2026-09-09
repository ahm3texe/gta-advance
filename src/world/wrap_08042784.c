/* Forwarding wrapper — 0x08042784-0x0804278F
 *
 * The body only calls FUN_08042790. Its purpose is UNKNOWN, so the
 * FUN_ name is retained rather than inventing unsupported semantics.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 * Arguments are already in r0-r3, so the wrapper does not touch them;
 * declaring it without arguments would still produce the same bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_08042784.c
 */

#include "gba_types.h"

extern void FUN_08042790(void);

/* 0x08042784 */
void FUN_08042784(void)
{
    FUN_08042790();
}
