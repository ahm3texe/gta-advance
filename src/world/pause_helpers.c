/* Pause helpers — 0x080664A4-0x080664EF
 *
 * Four small functions in the cluster. gVBlankEnabled (u16) is a state word;
 * gGameState[12] is the pause flag; 0x02036328 is an additional flag.
 *
 * The fourth (MaybeAdvance, 0x080664F0) was separated with a one-byte
 * difference: src/world/maybe_advance.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/pause_helpers.c
 */

#include "gba_types.h"

#define BASE_VALUE     (232 << 1)     /* 464, ROM `movs #232; lsls #1` */
#define STATE_ACTIVE   3

extern u16  gVBlankEnabled;
extern u8   gGameState[];
extern u8   gRam02036328;

extern void FUN_080657d8(u32 arg);

/* 0x080664A4 */
u32 GetBaseValue(void)
{
    return BASE_VALUE;
}

/* 0x080664AC */
void MaybeReset(void)
{
    if (gVBlankEnabled != 0)
        FUN_080657d8(2);
}

/* 0x080664C4 */
u32 IsSessionActive(void)
{
    if (gGameState[12] == 0)
        return 0;
    if (gVBlankEnabled != STATE_ACTIVE) {
        if (gRam02036328 == 0)
            return 0;
    }

    return 1;
}
