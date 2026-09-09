/* Script command: release the slot the operand maps to — 0x0805AA1C-0x0805AA2F
 *
 * FUN_08030D0C's answer goes straight into ReleaseSlot: the ROM does not touch
 * r0 between the two calls, so the second reads what the first left there.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_release_slot.c
 */

#include "gba_types.h"

extern u32 FUN_08030d0c(u16 id);
extern u32 ReleaseSlot(u32 index);

/* 0x0805AA1C */
u32 FUN_0805aa1c(u32 a, u32 id)
{
    ReleaseSlot(FUN_08030d0c(id));
    return 1;
}
