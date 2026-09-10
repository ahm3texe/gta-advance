/* Notify when the lookup resolves — 0x0803C698-0x0803C6B7
 *
 * FUN_0803B528's answer is narrowed to 16 bits for the call but tested at full
 * width, so a value whose low halfword is zero still counts as resolved.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/notify_if_resolved.c
 */

#include "gba_types.h"

extern u8 gRam02000F10[];

extern u32  FUN_0803b528(u32 value);
extern void FUN_0805945c(u32 mode, u16 id);

/* 0x0803C698 */
void FUN_0803c698(void)
{
    u32 found = FUN_0803b528(*(u32 *)gRam02000F10);

    if (found == 0)
        return;
    FUN_0805945c(0, found);
}
