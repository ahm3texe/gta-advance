/* Store at +0x1B0, reset five words, then step — 0x0801D818-0x0801D847
 *
 * 432 is `movs r2,#216 / lsls r2,#1`, so the +0x1B0 store goes through a
 * register add rather than a displacement.
 *
 * The third argument is kept in a callee-saved register across the first call
 * and only lands at +0x64 afterwards, which is what the `push {r4, r5}` pays
 * for.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/init_and_reset_fields.c
 */

#include "gba_types.h"

#define FAR_OFFSET  (216 << 1)  /* 0x1B0 */

typedef struct ResetBlock {
    u32 unk00;                  /* +0x00 */
    u32 unk04;                  /* +0x04 */
    u32 unk08;                  /* +0x08 */
    u32 unk0C;                  /* +0x0C */
    u32 unk10;                  /* +0x10 */
    u8  pad14[0x50];
    u32 owner;                  /* +0x64 */
} ResetBlock;

extern void FUN_0801a2c8(ResetBlock *block);
extern void FUN_0801d8a0(void *sub, u32 zero);

/* 0x0801D818 */
void FUN_0801d818(ResetBlock *block, u32 value, u32 owner)
{
    u32 offset = FAR_OFFSET;

    *(u32 *)((u8 *)block + offset) = value;
    FUN_0801a2c8(block);
    block->unk00 = 0;
    block->unk04 = 0;
    block->unk08 = 0;
    block->unk0C = 0;
    block->unk10 = 0;
    block->owner = owner;
    FUN_0801d8a0(block, 1);
}
