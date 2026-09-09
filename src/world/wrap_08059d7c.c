/* Forwarding wrapper — 0x08059D7C-0x08059D85
 *
 * The body only calls GetAnchorUnk30. Found with tools/find_wrappers.py;
 * the first scan skipped it because its target was unmapped. This pass
 * DISCOVERED the target and added it to the map.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_08059d7c.c
 */

#include "gba_types.h"

extern u32 GetAnchorUnk30(void);

/* 0x08059D7C */
u32 ForwardToGetAnchorUnk30(void)
{
    return GetAnchorUnk30();
}
