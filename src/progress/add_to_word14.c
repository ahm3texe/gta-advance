/* Add the argument to the +0x14 word — 0x08030B1C-0x08030B33
 *
 * The +0x14 word is the one docs/GRAM02025810_LAYOUT.md lists as read by
 * RunMenuScreen and CaptureSessionSnapshot. The sum goes to FUN_08030AE4 with a
 * constant 1, the same callee and second argument as the script handlers at
 * 0x08059F8C and 0x0805A024.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/add_to_word14.c
 */

#include "gba_types.h"

#define PROGRESS_WORD14  5       /* +0x14, as a u32 index */

extern u8 gRam02025810[];

extern void FUN_08030ae4(u32 value, u32 slot);

/* 0x08030B1C */
void FUN_08030b1c(u32 amount)
{
    u32 *progress = (u32 *)gRam02025810;

    FUN_08030ae4(progress[PROGRESS_WORD14] + amount, 1);
}
