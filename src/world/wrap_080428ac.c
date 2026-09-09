/* Forwarding wrapper — 0x080428AC-0x080428B7
 *
 * The body only calls FUN_08042900. Its purpose is UNKNOWN, so the
 * FUN_ name is retained rather than inventing unsupported semantics.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 * Arguments are already in r0-r3, so the wrapper does not touch them;
 * declaring it without arguments would still produce the same bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_080428ac.c
 */

#include "gba_types.h"

extern void FUN_08042900(void);

/* 0x080428AC */
void FUN_080428ac(void)
{
    FUN_08042900();
}
