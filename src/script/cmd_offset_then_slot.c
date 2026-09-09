/* Apply an offset, then select a slot — 0x0805A0C0-0x0805A0FB
 *
 * Does nothing and answers 0 while gFrameCounterEwram is still zero. Otherwise
 * FUN_080504B4 runs when GetBaseAlt is below the first operand OR that operand
 * is zero, and SetSlot always runs with the second.
 *
 * The comparison is UNSIGNED (`bcc`), unlike the signed GetBase test in
 * src/script/cmd_base_above.c, so this one takes the alternate getter's answer
 * as unsigned.
 *
 * The first operand lives in TWO callee-saved registers in the ROM
 * (`lsrs r4,r1,#16 / adds r5,r4,#0`): one is compared, the other passed on.
 * Two locals of the same type do not give that -- agbcc propagates one away.
 * What does is a `u32` local for the comparison and the `u16` PARAMETER itself
 * for the call: the two have different types, so the copy is a conversion and
 * survives.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_offset_then_slot.c
 */

#include "gba_types.h"

extern u32 gFrameCounterEwram;

extern u32  GetBaseAlt(void);
extern void FUN_080504b4(u32 offset);
extern void SetSlot(u32 slot);

/* 0x0805A0C0 */
u32 FUN_0805a0c0(u32 a, u16 offset, u16 slot)
{
    u32 limit = offset;

    if (gFrameCounterEwram == 0)
        return 0;
    if (GetBaseAlt() < limit) goto apply;
    if (limit != 0) goto skip;
apply:
    FUN_080504b4(offset);
skip:
    SetSlot(slot);
    return 1;
}
