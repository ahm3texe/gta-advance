/* Forwarding wrapper — 0x0804FFB4-0x0804FFBD
 *
 * The body only calls FUN_08046358. Its purpose is unknown, so its name
 * is unchanged. Found with tools/find_wrappers.py.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_0804ffb4.c
 */

#include "gba_types.h"

extern u32 FUN_08046358(void);

/* 0x0804FFB4 */
u32 FUN_0804ffb4(void)
{
    return FUN_08046358();
}
