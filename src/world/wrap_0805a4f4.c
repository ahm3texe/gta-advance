/* Forwarding wrapper — 0x0805A4F4-0x0805A4FD
 *
 * The body only calls HalvesEqual. Its purpose is unknown, so its name
 * is unchanged. Found with tools/find_wrappers.py.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_0805a4f4.c
 */

#include "gba_types.h"

extern u32 HalvesEqual(void);

/* 0x0805A4F4 */
u32 ForwardToHalvesEqual(void)
{
    return HalvesEqual();
}
