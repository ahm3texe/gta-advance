/* Flag arrays and triggers — 0x08031D24-0x08031D7D
 *
 * Two eight-entry u8 flag arrays: gFlagsA (0x02026F38) and gFlagsB
 * (0x02026EF0). The first function returns 1 if both arrays have nonzero
 * flags at any shared index. For each nonzero gFlagsA entry, the second
 * clears gBlockTable[i] (72-byte stride) through FUN_08013ABC and clears the flag.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/flag_arrays.c
 */

#include "gba_types.h"

#define ENTRY_COUNT   8
#define BLOCK_STRIDE  72

typedef struct Block {
    u8 pad[BLOCK_STRIDE];
} Block;

extern u8    gFlagsA[ENTRY_COUNT];
extern u8    gFlagsB[ENTRY_COUNT];
extern Block gBlockTable[ENTRY_COUNT];

extern void ReleaseObject(void *block);

/* 0x08031D24 */
u32 AnyPairSet(void)
{
    s32 i;

    for (i = 0; i <= ENTRY_COUNT - 1; i++) {
        if (gFlagsA[i] != 0) {
            if (gFlagsB[i] != 0)
                return 1;
        }
    }

    return 0;
}

/* 0x08031D54 */
void ClearAllBlocks(void)
{
    s32 i;
    u32 off;

    i = 0;
    off = 0;

    do {
        if (gFlagsA[i] != 0) {
            ReleaseObject((u8 *)gBlockTable + off);
            gFlagsA[i] = 0;
        }
        off += BLOCK_STRIDE;
        i++;
    } while (i <= ENTRY_COUNT - 1);
}
