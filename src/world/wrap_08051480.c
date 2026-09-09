/* Forwarding wrapper — 0x08051480-0x08051489
 *
 * The body only calls GetAnchorUnk04. Found with tools/find_wrappers.py;
 * the first scan skipped it because its target was unmapped. This pass
 * DISCOVERED the target and added it to the map.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a RETURN VALUE.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/wrap_08051480.c
 */

#include "gba_types.h"

extern u32 GetAnchorUnk04(void);

/* 0x08051480 */
u32 ForwardToGetAnchorUnk04(void)
{
    return GetAnchorUnk04();
}
