/* Forwarding wrapper — 0x08063BCC-0x08063BD5
 *
 * The body only calls FUN_08010224. Its purpose is unknown, so its name
 * is unchanged. Found with tools/find_wrappers.py.
 *
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_08063bcc.c
 */

#include "gba_types.h"

extern void FUN_08010224(void);

/* 0x08063BCC */
void FUN_08063bcc(void)
{
    FUN_08010224();
}
