/* Is the channel busy — 0x08033AD8-0x08033B07
 *
 * Index 0 is busy whenever the +0x2C byte is set. For any other index the
 * +0x2C byte is ignored and the answer is whether the index's word in the
 * +0x04 array is non-zero.
 *
 * The array base is the symbol PLUS FOUR through a register add, not a folded
 * pool constant, and the index is scaled before it is added.
 *
 * The 1 is a shared label reached BOTH ways: the index-0 path branches to it
 * and the array test falls into it, with the 0 after it. Written as an early
 * `return 0` the two answers come out the other way round.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/is_channel_busy.c
 */

#include "gba_types.h"

#define ARRAY_OFFSET  4

typedef struct AudioState {
    u8  pad00[0x10];
    u32 first;                  /* +0x10 */
    u8  pad14[0x10];
    u32 second;                 /* +0x24 */
    u32 third;                  /* +0x28 */
    u8  busy;                   /* +0x2C */
    u8  pad2D[3];
} AudioState;

extern AudioState gRam020001B0;
extern u8         gRam02027330[];

/* 0x08033AD8 */
u32 FUN_08033ad8(u32 index)
{
    u8 *base;
    u32 offset;

    if (gRam020001B0.busy != 0) {
        if (index == 0) goto yes;
    }
    base = gRam02027330;
    offset = index * 4;
    base += ARRAY_OFFSET;
    if (*(u32 *)(base + offset) == 0) goto no;
yes:
    return 1;
no:
    return 0;
}
