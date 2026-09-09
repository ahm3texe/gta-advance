/* Clear the RAM block and set its two ids
 * 0x08061D34-0x08061D47 and 0x08061D48-0x08061D57
 *
 * The clear is what fixes the block's size: 36 bytes, which is why
 * data/ram_map.csv records that and not the 24 the named fields reached.
 *
 * The setter has no prologue and returns through `bx lr`; it answers 1 without
 * having anything to fail at.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/reset_ram_block.c
 */

#include "gba_types.h"

typedef struct RamBlock {
    u32 unk00;                  /* +0x00 */
    u8  pad04[4];
    u8  type;                   /* +0x08 */
    u8  mode;                   /* +0x09 */
    u8  pad0A[10];
    u8  byte14;                 /* +0x14 */
    u8  pad15[1];
    u16 word16;                 /* +0x16 */
    u8  pad18[4];
    u16 word1C;                 /* +0x1C */
    u16 word1E;                 /* +0x1E */
    u8  pad20[4];               /* out to the 36 bytes 0x08061D34 clears */
} RamBlock;

extern RamBlock gRam02035EA0;

extern void *Memset(void *dest, int value, u32 count);

/* 0x08061D34 */
void FUN_08061d34(void)
{
    Memset(&gRam02035EA0, 0, sizeof(RamBlock));
}

/* 0x08061D48 */
u32 FUN_08061d48(u16 first, u16 second)
{
    gRam02035EA0.word1C = first;
    gRam02035EA0.word1E = second;
    return 1;
}
