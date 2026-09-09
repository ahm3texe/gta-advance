/* Draw an eight-digit counter with two-row tiles — 0x0802A610-0x0802A857
 *
 * Two near-identical bodies: find_twins.py reported 99.3% similarity; their
 * instructions share the same source pattern with shifted pool addresses
 * (WORKFLOW.md §10), and the loop-count difference noted below.
 *
 * Same tile scheme as DrawTwoDigits (draw_two_digits.c, 0x08031498): upper
 * 0xF8+d, lower 0x102+d, both ORed with palette nibble 0xF000; empty 0xF0E8.
 * Here there are eight digits, base 0x0600980E, and leading zeroes become
 * marker 10, then empty tiles.
 *
 * THREE MEASUREMENTS:
 * - Use increasing for (i=0; i<8; i++) with digits[i]. A descending counter
 *   and digits[7-i] emits the same instructions but moves movs r5,#7 one
 *   instruction early; walking a pointer gives 20 instructions differences.
 * - In the leading-zero loop, store the empty constant in a SEPARATE local;
 *   direct use computes the address before loading the constant.
 * - The leading-zero bound comparison must be SIGNED (ble); a pointer
 *   comparison emits bls.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/draw_eight_digits.c
 */

#include "gba_types.h"

extern s32 Div(s32 numerator, s32 denominator);

#define TILE_MAP_BASE   0x0600980E
#define TILE_ROW_STEP   64
#define DIGIT_TOP       0xF8
#define DIGIT_BOTTOM    0x102
#define DIGIT_BLANK     10
#define TILE_ATTR       0xF000
#define TILE_BLANK      0xF0E8

/* 0x0802A610 */
void DrawCounterDigits8(s32 value, s32 x, s32 y)
{
    s32 digits[8];
    s32 *p;
    vu16 *base;
    vu16 *top;
    vu16 *bot;
    s32 col;
    s32 rowoff;
    s32 d;
    s32 i;
    s32 blank;

    digits[7] = Div(value, 10000000);
    value -= digits[7] * 10000000;
    digits[6] = Div(value, 1000000);
    value -= digits[6] * 1000000;
    digits[5] = Div(value, 100000);
    value -= digits[5] * 100000;
    digits[4] = Div(value, 10000);
    value -= digits[4] * 10000;
    digits[3] = Div(value, 1000);
    value -= digits[3] * 1000;
    digits[2] = Div(value, 100);
    value -= digits[2] * 100;
    digits[1] = Div(value, 10);
    value -= digits[1] * 10;
    digits[0] = value;

    if (digits[7] == 0) {
        blank = DIGIT_BLANK;
        p = &digits[7];
        do {
            *p = blank;
            p--;
        } while ((s32)p > (s32)&digits[0] && *p == 0);
    }

    col = x * 2;
    rowoff = y * TILE_ROW_STEP;
    base = (vu16 *)TILE_MAP_BASE;
    top = (vu16 *)(col + ((u32)base + rowoff));
    rowoff = (y + 1) * TILE_ROW_STEP;
    bot = (vu16 *)(col + ((u32)base + rowoff));

    for (i = 0; i < 8; i++) {
        d = digits[i];
        if (d == DIGIT_BLANK) {
            *top-- = TILE_BLANK;
            *bot-- = TILE_BLANK;
        } else {
            *top-- = (DIGIT_TOP + d) | TILE_ATTR;
            *bot-- = (DIGIT_BOTTOM + d) | TILE_ATTR;
        }
    }
}

/* 0x0802A734 — same body except loop count 3 instead of 8 (movs r5,#2
 * instead of #7). Eight digits are still computed; only three are drawn.
 */
void DrawCounterDigits3(s32 value, s32 x, s32 y)
{
    s32 digits[8];
    s32 *p;
    vu16 *base;
    vu16 *top;
    vu16 *bot;
    s32 col;
    s32 rowoff;
    s32 d;
    s32 i;
    s32 blank;

    digits[7] = Div(value, 10000000);
    value -= digits[7] * 10000000;
    digits[6] = Div(value, 1000000);
    value -= digits[6] * 1000000;
    digits[5] = Div(value, 100000);
    value -= digits[5] * 100000;
    digits[4] = Div(value, 10000);
    value -= digits[4] * 10000;
    digits[3] = Div(value, 1000);
    value -= digits[3] * 1000;
    digits[2] = Div(value, 100);
    value -= digits[2] * 100;
    digits[1] = Div(value, 10);
    value -= digits[1] * 10;
    digits[0] = value;

    if (digits[7] == 0) {
        blank = DIGIT_BLANK;
        p = &digits[7];
        do {
            *p = blank;
            p--;
        } while ((s32)p > (s32)&digits[0] && *p == 0);
    }

    col = x * 2;
    rowoff = y * TILE_ROW_STEP;
    base = (vu16 *)TILE_MAP_BASE;
    top = (vu16 *)(col + ((u32)base + rowoff));
    rowoff = (y + 1) * TILE_ROW_STEP;
    bot = (vu16 *)(col + ((u32)base + rowoff));

    for (i = 0; i < 3; i++) {
        d = digits[i];
        if (d == DIGIT_BLANK) {
            *top-- = TILE_BLANK;
            *bot-- = TILE_BLANK;
        } else {
            *top-- = (DIGIT_TOP + d) | TILE_ATTR;
            *bot-- = (DIGIT_BOTTOM + d) | TILE_ATTR;
        }
    }
}
