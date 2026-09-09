/* Clamp to 0..450 and store at +0x02 — 0x08030A3C-0x08030A5F
 *
 * The bounds are a signed pair: `bge` for the floor and `ble` for the ceiling,
 * so the incoming value is an int. 450 is `movs #225 / lsls #1`, since Thumb's
 * movs immediate stops at 255.
 *
 * +0x02 is the halfword docs/GRAM02025810_LAYOUT.md records at High confidence,
 * reached here through a local u16 view because the symbol is shared as `u8 []`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/set_clamped_word02.c
 */

#include "gba_types.h"

#define VALUE_MAX  (225 << 1)

extern u8 gRam02025810[];

extern void FUN_0802a480(void);

/* 0x08030A3C */
void FUN_08030a3c(s32 value)
{
    u16 *progress;

    if (value < 0)
        value = 0;
    if (value > VALUE_MAX)
        value = VALUE_MAX;
    progress = (u16 *)gRam02025810;
    progress[1] = value;
    FUN_0802a480();
}
