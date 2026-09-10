/* Scale a 26-bit value into 0..9 — 0x0801981C-0x0801983B
 *
 * Adds 0x333333, keeps 26 bits, multiplies by five and takes the top bits, then
 * clamps to 9. The `(x << 2) + x` is agbcc's expansion of the multiply by five,
 * not something the source says.
 *
 * The clamp is a SIGNED comparison (`ble`), so the shifted value is an int.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/scale_to_ten.c
 */

#include "gba_types.h"

#define BIAS      0x333333
#define KEEP      0x03FFFFFF
#define STEPS     5
#define OUT_SHIFT 25
#define OUT_MAX   9

/* 0x0801981C */
s32 FUN_0801981c(u32 value)
{
    s32 scaled;

    value = value + BIAS;
    value = value & KEEP;
    scaled = value * STEPS >> OUT_SHIFT;
    if (scaled > OUT_MAX)
        scaled = OUT_MAX;
    return scaled;
}
