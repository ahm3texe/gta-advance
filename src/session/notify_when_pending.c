/* Notify the active slot when +0xA8 is set — 0x08061F9C-0x08061FBB
 *
 * The +0xA8 word of the mode record gates the call; 0x112 is the id handed to
 * FUN_08035058, built as `movs #137 / lsls #1` because it does not fit a Thumb
 * immediate.
 *
 * The record is reached through a pointer rather than a struct, because the
 * symbol is shared with src/world/is_mode_two.c, which uses only its first
 * byte, and the consistency check requires one extern type for it.
 *
 * Rule 65: the base has to go through a LOCAL. Applied to the symbol directly,
 * `gRam02036050 + 0xA8` is a constant and agbcc puts 0x020360F8 in the pool;
 * the ROM keeps 0x02036050 there and adds the 168 at run time.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/notify_when_pending.c
 */

#include "gba_types.h"

#define PENDING_OFFSET  0xA8
#define NOTIFY_ID       (137 << 1)

extern u8 gRam02036050[];

extern u32  GetActiveSlot(void);
extern void FUN_08035058(u32 slot, u32 id);

/* 0x08061F9C */
void FUN_08061f9c(void)
{
    u8 *record = gRam02036050;
    u32 *pending;

    pending = (u32 *)(record + PENDING_OFFSET);
    if (*pending != 0)
        FUN_08035058(GetActiveSlot(), NOTIFY_ID);
}
