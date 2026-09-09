/* Historical HUD tile-writer / 4bpp strip band — 0x08030B40 .. 0x08031A1C
 *
 * Matching neighbors: hud_fields.c (0x08030B60, 0x08030E78, 0x08030F28),
 * area_cleanup.c (0x08030CB4), area_flags.c. They establish empty tile
 * 0xF0E8 and the 32-entry/64-byte tilemap row stride.
 *
 * HISTORICAL PLACEMENT WARNING, before these functions were split:
 * tools/agbcc_build.py links all functions in one C file consecutively from
 * its lowest address (then 0x08030B40). Intervening ROM functions mean later
 * bodies land at the wrong address, corrupting bl offsets even with correct
 * source. Affected: SendTextMode2 (3/12 bytes) and DrawTwoDigits (branch offset).
 * Individually compiled at their own addresses with verify_c_function.py:
 *   SendTextMode2 12 BYTE-MATCHING (0x080315C0)
 *   DrawTwoDigits 156 BYTE-MATCHING (0x08031498)
 * Functions without bl (0x08030F50, 0x080311DC, 0x08031328, 0x08031A1C)
 * remained position-independent. hud_fields.c likewise has 752/136-byte
 * gaps but no calls, so was unaffected.
 *
 * 0x0803173E: INVALID FUNCTION BOUNDARY; no C was written for this entry.
 * Evidence:
 * 1. First instruction adds r4,#1, with no prologue.
 * 2. b.n 0x80316CE at 0x080317DC branches BEFORE the recorded start.
 * 3. Epilogue at 0x080317DE: pop {r3,r4,r5} / mov r8..sl / pop {r4-r7} /
 *    pop {r0} / bx r0. Its prologue is at 0x080316B0:
 *    push {r4,r5,r6,r7,lr} / mov r7,sl / mov r6,r9 / mov r5,r8 /
 *    push {r5,r6,r7} / sub sp,#4.
 * 4. 0x08031684-0x080316AF is a separate complete function (push {r4,lr}
 *    ... bx r0, pool word 0x02025810 at 0x080316AC), then missing from
 *    data/functions.csv.
 * The 186-byte gap contained two functions. The real boundary is
 * 0x080316B0-0x080317EE (318 bytes); 0x0803173E lies inside its body.
 * It is a sibling of 0x08031A1C's eight-nibble masked strip renderer,
 * differing in row count and parameter placement.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm (docs/COMPILER.md)
 * Verification: make c-match FILE=src/ui/draw_two_digits.c
 */

#include "gba_io.h"
#include "gba_types.h"

#define TILE_BLANK   0xF0E8

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 text, u32 kind);

/* 0x08031498 — BYTE-MATCHING when compiled at its own address; see the
 * historical placement warning. Draw a two-digit number right-to-left with
 * two-row digit tiles. If flag bit0 is set, hide a zero tens digit with
 * the empty tile.
 *
 * Tiles: upper 0xF8+d, lower 0x102+d, ORed with 0xF000. vu16 stores narrow
 * the constant to HImode, placing -0x1000 (0xFFFFF000) in the pool exactly
 * as in the ROM. Empty tile 0xF0E8 is stored directly without OR.
 *
 * MEASURED address construction, accounting for 21 instructions differences:
 * - Separate base local required. A flat (y*64+0x06009802)+col lets agbcc
 *   reassociate col+BASE as a common subexpression and hoist one ldr (55/75).
 *   A local makes only BASE common, producing adds r0,r0,r1 + adds r4,r2,r0.
 * - rowoff must also be a separate statement; otherwise the base ldr appears
 *   before lsls r0,r6,#6 (74/75, one instruction).
 * - Final addition must be col+(...), not (...)+col: ROM needs adds r4,r2,r0
 *   rather than adds r4,r0,r2.
 * Rejected: (vu16*)(y*S+B)+x, col+(y*S+B), (y*S+B)+col,
 * &((vu16*)B)[y*32+x], two separate row pointers (52-69/75).
 */
extern s32 Div(s32 numerator, s32 denominator);

#define TILE_MAP_BASE   0x06009802
#define TILE_ROW_STEP   64
#define DIGIT_TOP       0xF8
#define DIGIT_BOTTOM    0x102
#define DIGIT_BLANK     10
#define TILE_ATTR       0xF000

void DrawTwoDigits(s32 value, s32 x, s32 y, s32 flags)
{
    s32 digits[8];
    vu16 *base;
    vu16 *top;
    vu16 *bot;
    s32 col;
    s32 rowoff;
    s32 d;
    s32 i;

    digits[1] = Div(value, 10);
    digits[0] = value - digits[1] * 10;
    if ((flags & 1) && digits[1] == 0)
        digits[1] = DIGIT_BLANK;

    col = x * 2;
    rowoff = y * TILE_ROW_STEP;
    base = (vu16 *)TILE_MAP_BASE;
    top = (vu16 *)(col + ((u32)base + rowoff));
    rowoff = (y + 1) * TILE_ROW_STEP;
    bot = (vu16 *)(col + ((u32)base + rowoff));

    for (i = 0; i < 2; i++) {
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

