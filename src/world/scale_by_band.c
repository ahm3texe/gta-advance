/* Scale by distance band — 0x0806549C-0x08065517
 *
 * Divide the key into eight bands with 8.8 fixed-point factors (256 = 1.0):
 * 281, 256, 230, 204, 179, 153, 128, 102, an attenuation curve descending
 * roughly from 1.1 to 0.4.
 *
 * The ROM reverses the last two bands: load 102 first, then overwrite with
 * 128 if the bound is not exceeded. That follows naturally from
 * `else if (key <= 0x4FFFF) 128 else 102`. The comparisons are unsigned (bhi),
 * but the product shifts arithmetically (asrs): key is u32, value is s32.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/scale_by_band.c
 */

#include "gba_types.h"

#define FIXED_SHIFT 8

/* 0x0806549C */
s32 ScaleByDistanceBand(s32 value, u32 key)
{
    s32 factor;

    if (key <= 0x6FFF)
        factor = 281;
    else if (key <= 0x7FFF)
        factor = 256;
    else if (key <= 0xFFFF)
        factor = 230;
    else if (key <= 0x14FFF)
        factor = 204;
    else if (key <= 0x24FFF)
        factor = 179;
    else if (key <= 0x34FFF)
        factor = 153;
    else if (key <= 0x4FFFF)
        factor = 128;
    else
        factor = 102;

    return (value * factor) >> FIXED_SHIFT;
}
