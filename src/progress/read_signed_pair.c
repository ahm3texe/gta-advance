/* Read the signed +0x04 and +0x05 bytes — 0x08030B88-0x08030B9B
 *
 * Both fields are SIGNED, and the ROM says so directly: Thumb has no
 * immediate-offset form of `ldrsb`, so the offsets go through a register
 * (`movs r2,#4 / ldrsb r2,[r3,r2]`). docs/GRAM02025810_LAYOUT.md records that
 * this idiom is the block's only source of signedness information.
 *
 * The two answers are widened to 32 bits by the stores, which is why the
 * outputs are s32 and not s8.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/read_signed_pair.c
 */

#include "gba_types.h"

extern u8 gRam02025810[];

/* 0x08030B88 */
void FUN_08030b88(s32 *first, s32 *second)
{
    s8 *progress = (s8 *)gRam02025810;

    *first = progress[4];
    *second = progress[5];
}
