/* HUD tile writers + the 4bpp strip blitter band — 0x08030B40 .. 0x08031A1C
 *
 * Matching files in the same neighborhood: src/ui/hud_fields.c (0x08030B60,
 * 0x08030E78, 0x08030F28), src/world/area_cleanup.c (0x08030CB4),
 * src/world/area_flags.c.  The blank tile constant 0xF0E8 and the row stride
 * of the 32-entry (64-byte) tile map come from there.
 *
 * ---------------------------------------------------------------------
 * LAYOUT WARNING (for the two functions in this file)
 * ---------------------------------------------------------------------
 * tools/agbcc_build.py links ALL functions of a C file CONSECUTIVELY, starting
 * from the SMALLEST address in the file (here 0x08030B40).  Because there are
 * other functions between them in the ROM, the second and later functions DO
 * NOT LAND on their own ROM addresses.  That breaks the branch offset in
 * functions containing a `bl` -- even when the source is correct.
 * Affected: SendTextMode2 (3/12 bytes) and DrawTwoDigits (branch offset).
 * Both are BYTE-MATCHING when compiled at THEIR OWN addresses; measured:
 *     one function per file -> `python3 tools/verify_c_function.py <file>`
 *     SendTextMode2  12  BYTE-MATCHING  (0x080315C0)
 *     DrawTwoDigits 156  BYTE-MATCHING  (0x08031498)
 * The others (0x08030F50, 0x080311DC, 0x08031328, 0x08031A1C) contain no `bl`,
 * so they are position-independent and match in this file too.
 * The same situation exists in src/ui/hud_fields.c (gaps of 752 and 136 bytes)
 * -- there no function makes a call, so it causes no trouble.
 *
 * ---------------------------------------------------------------------
 * 0x0803173E — WRONG BOUNDARY, NOT A FUNCTION (measured, no C written)
 * ---------------------------------------------------------------------
 * Evidence:
 *  1. The first instruction at 0x0803173E is `adds r4,#1`; there is no prologue.
 *  2. The `b.n 0x80316CE` at 0x080317DC branches BACKWARDS, to BEFORE the
 *     recorded start -- so the body begins before 0x0803173E.
 *  3. The epilogue at 0x080317DE is `pop {r3,r4,r5} / mov r8..sl / pop {r4-r7} /
 *     pop {r0} / bx r0`; the matching prologue is at 0x080316B0:
 *     `push {r4,r5,r6,r7,lr} / mov r7,sl / mov r6,r9 / mov r5,r8 /
 *      push {r5,r6,r7} / sub sp,#4`.
 *  4. 0x08031684-0x080316AF is a SEPARATE and complete function
 *     (`push {r4,lr}` ... `bx r0`, with the pool word 0x02025810 at
 *     0x080316AC) -- not recorded in data/functions.csv at all.
 * Conclusion: the 186-byte "gap" is really two functions; the true boundary is
 * 0x080316B0-0x080317EE (318 bytes) and 0x0803173E sits in the middle of its
 * body.  No C was written for 0x0803173E.  (The real function is the sibling
 * of 0x08031A1C: the same eight-nibble masked strip blitter, differing only in
 * row count and parameter layout.)
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/video/blit_strip_4bpp.c
 */

#include "gba_io.h"
#include "gba_types.h"

#define TILE_BLANK   0xF0E8

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 text, u32 kind);

/* --------------------------------------------------------------------
 * 0x08031A1C — BYTE-MATCHING, 470 bytes.  4bpp (one pixel per nibble)
 * strip blitter.
 *
 * 48 rows; on each row eight nibbles are read and packed into two halfwords
 * (dst advances 4 bytes per row).  A row falling outside the vertical clip
 * range (row < 0 or row >= rowLimit) takes the FAST path: eight nibbles
 * straight from the source.  Inside the range each nibble is tested
 * separately; while the column counter (col) is negative it is blended with
 * the image underneath: (*mask & *under) | *src.
 * Pointer strides: src and under advance 48 bytes per row, mask advances
 * 8 + maskStep.
 *
 * MEASURED (the road from 157 differences to 0):
 *  1. The loop must be written DESCENDING.  The ROM emits `movs #48 / ... /
 *     add r9,-1 / cmp #0 / beq` and RECOMPUTES the row as
 *     `(rowBase + 48) - counter` (reloading `rowBase` from the stack every
 *     iteration).  `for (i = 0; i < 48; i++)` -- s32 or u32, for or do-while,
 *     it makes no difference -- stays ASCENDING in agbcc (`add r0,r9` +
 *     `cmp #0x2f`); rule 42 does not apply here.  Writing
 *     `row = rowBase + (48 - i)` reproduces the ROM's three instructions
 *     exactly; `(rowBase + 48) - i` and `rowBase + 48 - i` refold into
 *     `rowBase - (i - 48)` (82/238).
 *  2. The clip test must be a SINGLE `if (a || b)`.  Because one bound is a
 *     variable, the folding of rule 60 does not happen; agbcc emits
 *     `bge FAST / bge MASKED / (fallthrough) FAST` -- and that is the ROM's
 *     block order.
 *  3. The order in `(*mask & *under)` MATTERS: agbcc loads the SECOND operand
 *     of an AND first (the `&` counterpart of rule 53).  Writing
 *     `(*under & *mask)` moves `ldrb [r5]` to the front (84 -> 87
 *     instruction differences).
 *  4. On the FAST path, mask/under must be incremented PER NIBBLE.  The ROM
 *     shows a single `adds r5,#8 / adds r4,#8`, and the first reflex is to
 *     write that in the source; doing so shifts the lifetimes of the n0..n3
 *     temporaries and flips the register priorities BY A HAIR (col is
 *     allocated r7 and n2 r6, whereas the ROM has col r6 and n2 r7).  Writing
 *     eight separate `mask++/under++` gives the ROM's allocation; merging the
 *     increments into a single `+= 8` is something the compiler does ITSELF,
 *     AFTER allocation.
 *     Measured priorities (tools/dump_alloc.py):
 *         wrong form:  n2 1.383 (r6) > col 1.358 (r7)
 *         right form:  col 1.358 (r6) > n2 1.258 (r7)
 *     ELIMINATED intermediate forms: `mask += 8` at the start/middle/end of
 *       the block (87..197), `mask += 4` per halfword (196..233), splitting
 *       the temporaries per branch (78..80), the type and declaration order
 *       of the temporaries (no effect), and the lifetime-extending no-op
 *       `n2++; n2--;` (rule 50) -- that one ALSO matches exactly, but it is
 *       not defensible source, so it was not taken.
 * ------------------------------------------------------------------ */
#define STRIP_ROWS   48
#define ROW_BYTES    40

void BlitStrip4bpp(s32 rowBase, s32 colBase, s32 rowLimit, s32 maskStep,
                  const u8 *mask, const u8 *under, u16 *dst, const u8 *src)
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
            if (col++ >= 0) {
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
