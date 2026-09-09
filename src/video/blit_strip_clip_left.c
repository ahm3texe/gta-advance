/* 8x48 4bpp strip blitter, LEFT clipping — 0x08031844-0x08031A1B
 *
 * THE SAME SOURCE as 0x08031A1C in src/video/blit_strip_4bpp.c; the only
 * difference is the direction of the column test.
 *
 * HOW IT WAS FOUND (do this before starting a new attempt): the instruction
 * listings of the two ROM bodies were compared --
 *     python3 tools/disasm_function.py 0x08031844 > a
 *     python3 tools/disasm_function.py 0x08031A1C > b
 *     diff a b        # only the BRANCH instructions differ
 * All 235 instructions are the same; the only differences are the branch
 * targets and the condition of the column test (`blt` -> `bge`). So 0x08031A1C
 * takes the plain source while `col >= 0`, and this function takes it while
 * `col < 0` -- the opposite edge of the strip. In the source that is a single
 * character: `if (col++ >= 0)` -> `if (col++ < 0)`.
 *
 * The lesson generalises: if a function does not match and a sibling of nearly
 * THE SAME SIZE in the ROM already does, diff the two bodies against each
 * other first. Check that before chasing register allocation.
 *
 * Measurements inherited from the sibling (all hold here too):
 *  1. The loop is DESCENDING; the row is RECOMPUTED as `rowBase + (48 - i)`.
 *  2. The vertical clip test is a SINGLE `if (a || b)`.
 *  3. The order in `(*mask & *under)` matters: agbcc loads the SECOND operand
 *     of the AND first.
 *  4. On the FAST path, mask/under must be incremented PER NIBBLE; the ROM's
 *     single `+= 8` is something the compiler produces itself, AFTER
 *     allocation.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/video/blit_strip_clip_left.c
 */

#include "gba_types.h"

#define STRIP_ROWS   48
#define ROW_BYTES    40

/* 0x08031844 */
void BlitStripClipLeft4bpp(s32 rowBase, s32 colBase, s32 rowLimit,
                           s32 maskStep, const u8 *mask, const u8 *under,
                           u16 *dst, const u8 *src)
{
    s32 i;
    s32 row;
    s32 col;
    u32 n0;
    u32 n1;
    u32 n2;
    u32 n3;

    for (i = STRIP_ROWS; i != 0; i--) {
        col = colBase;
        row = rowBase + (STRIP_ROWS - i);
        if (row >= rowLimit || row < 0) {
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        } else {
            if (col++ < 0) {
                n0 = *src;
                src++;
                mask++;
                under++;
            } else {
                n0 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n1 = *src;
                src++;
                mask++;
                under++;
            } else {
                n1 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n2 = *src;
                src++;
                mask++;
                under++;
            } else {
                n2 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n3 = *src;
                src++;
                mask++;
                under++;
            } else {
                n3 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            if (col++ < 0) {
                n0 = *src;
                src++;
                mask++;
                under++;
            } else {
                n0 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n1 = *src;
                src++;
                mask++;
                under++;
            } else {
                n1 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n2 = *src;
                src++;
                mask++;
                under++;
            } else {
                n2 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n3 = *src;
                src++;
                mask++;
                under++;
            } else {
                n3 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        }
        mask += maskStep;
        under += ROW_BYTES;
        src += ROW_BYTES;
    }
}
