/* Masked comparison — 0x08023A80-0x08023A9B
 *
 * Apply the same mask to the word at 0x02000224 and object +0x0C. Return 1
 * if DIFFERENT, otherwise 0.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/masked_compare.c
 */

#include "gba_types.h"

typedef struct MaskedNode {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} MaskedNode;

extern u32 gRam02000224;

/* 0x08023A80 */
u32 MaskedDiffers(MaskedNode *node, u32 mask)
{
    u32 current;
    u32 wanted;

    current = gRam02000224 & mask;
    wanted = node->flags & mask;
    if (current == wanted)
        return 0;
    return 1;
}
