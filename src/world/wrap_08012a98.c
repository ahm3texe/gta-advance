/* Forwarding wrapper — 0x08012A98-0x08012AA3
 *
 * The body only calls InitNodePool. Its purpose is UNKNOWN, so the
 * FUN_ name is retained rather than inventing unsupported semantics.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 * Arguments are already in r0-r3, so the wrapper does not touch them;
 * declaring it without arguments would still produce the same bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_08012a98.c
 */

#include "gba_types.h"

extern void InitNodePool(void);

/* 0x08012A98 */
void ForwardToInitNodePool(void)
{
    InitNodePool();
}
