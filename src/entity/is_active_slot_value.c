/* Is the argument the active slot's value — 0x0803809C-0x080380B3
 *
 * Rule 72: the result variable is introduced after the call. The `push {r4}`
 * this one does have is for the argument, which must survive the call.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/is_active_slot_value.c
 */

#include "gba_types.h"

extern u32 GetActiveSlotValue(void);

/* 0x0803809C */
u32 FUN_0803809c(u32 value)
{
    u32 active = GetActiveSlotValue();
    u32 result = 0;

    if (value == active)
        result = 1;
    return result;
}
