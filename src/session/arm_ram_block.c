/* Arm the RAM block when nothing is pending — 0x080625A0-0x080625CF
 *
 * Runs only when the block's first word is set AND the progress block's +0x04
 * halfword is clear. It then calls FUN_08060F60 and writes a fixed pair into
 * the block: 75 into the +0x14 byte and 617 into the +0x16 halfword.
 *
 * 617 comes through the literal pool; 75 fits a `movs`. The ROM materialises
 * BOTH before either store, and in that order, so both are locals assigned
 * ahead of the two writes. Written inline the pool load lands between the
 * stores; with only the 617 in a local it lands before the 75.
 *
 * The progress block is reached through a local pointer for the reason
 * src/script/cmd_progress_minus.c records: a cast applied to the symbol folds
 * the +0x04 into the pool constant, and the ROM keeps the displacement on the
 * load. The block's own address stays in r4 across the call, which is what the
 * `push {r4}` pays for.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/arm_ram_block.c
 */

#include "gba_types.h"

#define ARM_BYTE   75
#define ARM_WORD   617

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
extern u8       gRam02025810[];

extern void FUN_08060f60(void);

/* 0x080625A0 */
void FUN_080625a0(void)
{
    u16 *progress;
    u8  byte;
    u16 word;

    if (gRam02035EA0.unk00 == 0)
        return;
    progress = (u16 *)gRam02025810;
    if (progress[2] != 0)
        return;
    FUN_08060f60();
    byte = ARM_BYTE;
    word = ARM_WORD;
    gRam02035EA0.byte14 = byte;
    gRam02035EA0.word16 = word;
}
