/* Forwarding wrapper over CallWithOffset — 0x08051458-0x08051461
 *
 * Rule 35: `pop {r0}; bx r0` overwrites r0, so the return type is void, which
 * matches CallWithOffset's own signature in src/world/offset_helpers.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/wrap_call_with_offset.c
 */

#include "gba_types.h"

extern void CallWithOffset(u32 offset);

/* 0x08051458 */
void FUN_08051458(u32 offset)
{
    CallWithOffset(offset);
}
