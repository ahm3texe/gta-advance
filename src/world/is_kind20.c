/* Test kind 20 with a jump table — 0x08067404-0x080674AD
 *
 * Return 1 only for kind 20; other values in 0..35 and values above 35 return
 * 0. The source uses a 36-case switch, not kind == 20: the ROM has cmp #35 /
 * bhi followed by a 36-word JUMP TABLE, with 35 entries targeting return 0.
 *
 * MEASURED: each value needs its OWN case label to generate the table. GNU
 * ranges (case 0 ... 19:) collapse to two comparisons and remove it. Grouped
 * or individual labels, and placing case 20 first or last, make no difference:
 * all five forms match exactly.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/is_kind20.c
 */

#include "gba_types.h"

#define KIND_SPECIAL 20

/* 0x08067404 */
u32 IsKind20(u32 kind)
{
    switch (kind) {
    case KIND_SPECIAL:
        return 1;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:        return 0;
    default:
        return 0;
    }
}
