/* Forwarding wrapper — 0x08051464-0x0805146D
 *
 * The body only calls FUN_080504b4. Its purpose is unknown, so its name
 * is unchanged. Found with tools/find_wrappers.py.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_08051464.c
 */

#include "gba_types.h"

extern void FUN_080504b4(void);

/* 0x08051464 */
void FUN_08051464(void)
{
    FUN_080504b4();
}
