/* Slot range constants — 0x0803C178-0x0803C1B3
 *
 * Two functions. Continuing slot_config.c, the first returns range at slot
 * +16 for which=1/2, or default 0x640000 otherwise. The second always
 * returns 0x640000.
 *
 * 0x640000 = 200 << 15 (ROM movs #200; lsls #15), approximately 100 meters.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_range.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define PICK_A          1
#define PICK_B          2
#define DEFAULT_RANGE   0x640000        /* 200 << 15 */

typedef struct SlotHead {
    u8  pad00[0x10];
    u32 range;                  /* +0x10 */
} SlotHead;

extern u8       gGameState[];

/* 0x0803C178 */
u32 GetSlotRange(u32 which)
{
    if (which == PICK_A)
        return ((SlotHead *)gRam02000F10)->range;

    if (which == PICK_B) {
        if (gGameState[12] != 0)
            return ((SlotHead *)gRam02001140)->range;
    }

    return DEFAULT_RANGE;
}

/* 0x0803C1AC */
u32 GetDefaultRange(void)
{
    return DEFAULT_RANGE;
}
