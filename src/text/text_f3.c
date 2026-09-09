/* Clear the remainder of a text row — 0x08064180-0x0806430B (396 bytes)
 *
 * DMA3 clears everything to the RIGHT of (x,y) on this text row. The layer
 * uses 8x8 8bpp tiles, 64 bytes each, with 30 tiles/240 pixels across.
 *   dest = gTextVramBase + col*64 + (gTextRowStride * row)*64
 * This is the same addressing pattern as ClearTextArea in clear_text_area.c.
 *
 * Two stages:
 * 1. If sub != 0, clear only the right part of the current tile. Its eight
 *    rows are eight bytes apart, requiring eight DMA transfers of 8-sub bytes,
 *    advancing the destination by eight each time.
 * 2. Clear remaining full tiles col..29 in one contiguous (30-col)*64-byte DMA.
 * If gHalfLineSpacing is nonzero, finish after eight pixels of height. If
 * zero, repeat at row+1 for 16-pixel line height, consistently with
 * clear_text_area.c/set_text_context.c. Increment odd x on entry to align
 * halfword stores; sub is then even and (sub>>1)*2 == sub.
 *
 * DMA control 0x81000000 enables bit 31 and fixed-source bit 24 for 16-bit
 * transfers. The source is one stack zero; counts are halfwords, half the
 * byte count. Write source and destination BEFORE control, which starts DMA.
 *
 * NON-MATCHING, PARKED: best output 392 bytes vs ROM 396; 132/198 instructions
 * match, 66 differ. Structure, branches, constants, shift/division patterns
 * and loop order match. All 19 instructions in full-tile blocks two/four
 * match, as do entry clipping, address setup, five loop-preheader instructions
 * (movs r0,#64 / adds r0,r0,r6 / mov r9,r0 / adds r5,#1 / mov sl,r5), and
 * the loop tail.
 *
 * CORRECTED DIAGNOSIS: the earlier claim that x lands in ip is obsolete.
 * adds r7,r0,#0 now matches exactly; x is in r7 on both sides.
 * The measured root cause is allocation of loop-live values:
 *   value          ROM          ours
 *   DMA control    r3           stack sp+12 (critical)
 *   IME base       ip           r6
 *   zero           r8           ip
 *   &fill          stack sp+12  r8
 *   DMA3 base      r5           r3
 *   counter i      r4           r5
 * The ROM loop has 13 instructions, ours 12: high-register IME base requires
 * two mov r0,ip per iteration, while low r6 needs none. One extra instruction
 * in each of two loops explains 396-392 = 4 bytes; no code is missing.
 *
 * CONTROL SPILL (dump_alloc.py):
 *   p67 first-half control: refs 5, lifetime 18, floor_log2(5)*5/18=0.556,
 *       order 7 -> SPILL.
 *   p128 second-half equivalent: same priority, order 8 -> r4.
 *   p56 DMA3 base: refs 9, lifetime 48, floor_log2(9)*9/48=0.5625, order 5 -> r3.
 * DMA3 ranks just above control. p67's conflict set leaves r4-r7 available
 * (no conflict with p24/dest or p26/col), yet it spills while p128 gets r4.
 * The investigation attributed this to a global_alloc cost decision beyond
 * conflicts; rule-50 refs/lifetime adjustments below failed.
 *
 * ONLY IMPROVEMENT IN THIS SESSION (70 -> 66 differing instructions): remove
 * rem and embed TILE_WIDTH-sub directly in the loop control expression.
 * Invariant motion then places 8-sub after invariant loads in the preheader
 * as a short-lived temporary, matching movs r0,#8 / subs r0,r0,r4. Separate
 * rem creates a long-lived r4 allocno and shifts the preheader (inverse rule 69).
 *
 * REJECTED THIS SESSION (~1200 builds; score = differing instructions, base 66):
 * - 720 preheader statement permutations (pairs/nxt/ncol/rem/dst): best order
 *   nxt,ncol,pairs,dst gives 66; runner-up 68, others 70-90. Axis exhausted.
 * - Advance dest in place instead of separate dst: no order below 70.
 * - Derive dst from the counter as dest+pairs*2+(TILE_ROWS-1-i)*TILE_WIDTH
 *   and four related forms: >=75. ROM adds r2,#8 needs explicit dst += TILE_WIDTH.
 * - Local control value to increase refs: preheader 77, loop 86, shared
 *   full-tile control local 77. Inline expression is correct for this axis.
 * - Permute fill/src/dst/ctl while writing control LAST: all 66. Two orders
 *   writing control early score 65 but start DMA with the old source and
 *   are semantically WRONG; rejected.
 * - Cast REG_DMA3.src = (const void *)&fill: no change. volatile u16 *fillp: 76.
 * - Remove pairs: still 66; retained for readability.
 *
 * EARLIER REJECTIONS:
 * - 401 declaration permutations of ten locals: not one byte changes.
 *   Confirms COMPILER.md: declaration order does not determine stack layout.
 * - static __inline__ DmaFill helper: 149 differing instructions, parameter copies.
 * - Entirely separate second-half locals: fill2 adds a stack slot, giving 142.
 * - volatile DmaChannel *dma (ClearTextArea form): 90; ROM reloads the base
 *   per block, so macros are appropriate here.
 * - volatile u16 fill[2] (rule 20): 104, output 412 bytes; scalar fill is correct.
 * - Increasing for (i=0; i<8; i++) (rule 42): 119. ROM movs r4,#7 / cmp / bge
 *   requires descending for; i=8; do {} while (--i) gave the same result.
 * - Swap fill=0 and REG_IME=0: +11.
 * - Do not recompute col/sub in the second half: 85; ROM recomputes both.
 * - Force register s32 x asm("r7"): 74, REJECTED by WORKFLOW.md §6 and
 *   review_c_source.py. It hides the explanation and is now unnecessary:
 *   x naturally receives r7.
 *
 * THREE EARLIER IMPROVEMENTS (100 -> 70):
 * 1. Separate ime per block (rule 69) matches all 19 full-tile instructions
 *    and assigns dest to r6.
 * 2. nxt/ncol intermediates put the five preheader instructions
 *    (movs #64 ... mov sl,r5) in ROM order.
 * 3. Write the control constant INSIDE the loop (rule 21); invariant motion
 *    places it at the END of the preheader, as in the ROM.
 *
 * Suggested next step in this investigation: focus permuter exploration on
 * preheader invariant-load order. The remaining allocation decision concerns
 * which of control/IME base receives a low register; tested source-level
 * adjustments were exhausted.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/text_f3.c
 */

#include "gba_io.h"

#define SCREEN_RIGHT      239   /* 240 pixels wide */
#define SCREEN_BOTTOM     159   /* 160 pixels high */
#define TILE_SHIFT          3   /* pixel -> tile column */
#define TILE_WIDTH          8   /* pixels per tile */
#define TILE_ROWS           8   /* rows per tile */
#define TILE_BYTES         64   /* one 8x8 8bpp tile */
#define TILE_BYTE_SHIFT     6   /* << 6 == * TILE_BYTES */
#define TILE_COLUMNS       30   /* 240 / 8 */
#define LAST_TILE_COL      29

/* DMA3 control: enabled (bit 31), fixed source (bit 24), 16-bit. */
#define DMA_FILL_HALFWORDS 0x81000000

extern u8  *gTextVramBase;     /* 0x02036310 */
extern u32  gTextRowStride;    /* 0x0203630C — row stride in tiles */
extern u32  gHalfLineSpacing;  /* 0x0203631C — nonzero means one tile row */

/* 0x08064180 */
void FUN_08064180(s32 x, s32 y)
{
    volatile u16 fill;   /* DMA fixed source; stack sp+0 */
    u8 *dest;            /* byte address of the first tile to clear */
    u8 *dst;             /* partial-tile loop cursor */
    u8 *nxt;             /* full tile following the partial tile */
    s32 row;
    s32 col;
    s32 ncol;            /* column following the partial tile */
    s32 sub;             /* x pixel offset within its tile */
    s32 pairs;           /* pixel pairs to skip */
    s32 i;
    s32 control;

    /* 8bpp halfword writes require an even starting pixel. */
    if ((x & 1) != 0)
        x++;
    if ((u32)x > SCREEN_RIGHT)
        return;
    if ((u32)y > SCREEN_BOTTOM)
        return;

    row = y >> TILE_SHIFT;
    col = x >> TILE_SHIFT;
    sub = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_BYTE_SHIFT)
         + (gTextRowStride * row << TILE_BYTE_SHIFT);

    /* Upper tile row, stage one: partial tile. Rows are eight bytes apart,
 * so use eight transfers rather than one.
 */
    if (sub != 0) {
        u16 ime;

        nxt = dest + TILE_BYTES;
        ncol = col + 1;
        pairs = sub >> 1;
        dst = dest + pairs * 2;
        for (i = TILE_ROWS - 1; i >= 0; i--) {
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = &fill;
            REG_DMA3.dst = dst;
            /* Rule 21: write the constant INSIDE the loop; invariant motion places
 * it last in the preheader, as in the ROM. Keep TILE_WIDTH-sub here too;
 * a separate rem local disrupts allocation (see header).
 */
            REG_DMA3.control = ((TILE_WIDTH - sub) / 2) | DMA_FILL_HALFWORDS;
            REG_DMA3.control;
            REG_IME = ime;
            dst += TILE_WIDTH;   /* one tile row = 8 bytes */
        }
        dest = nxt;
        col = ncol;
    }

    /* Stage two: remaining full tiles are contiguous, so clear one block. */
    if (col <= LAST_TILE_COL) {
        u16 ime;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        REG_DMA3.src = &fill;
        REG_DMA3.dst = dest;
        control = (((TILE_COLUMNS - col) << TILE_BYTE_SHIFT) >> 1)
                | DMA_FILL_HALFWORDS;
        REG_DMA3.control = control;
        REG_DMA3.control;
        REG_IME = ime;
    }

    /* Half line height: do not clear the lower tile row. */
    if (gHalfLineSpacing != 0)
        return;

    /* The ROM recomputes col/sub. Caching them leaves 85 differing
 * instructions; see rejected attempts above.
 */
    col = x >> TILE_SHIFT;
    sub = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_BYTE_SHIFT)
         + (gTextRowStride * (row + 1) << TILE_BYTE_SHIFT);

    /* Lower tile row: repeat the same two stages. */
    if (sub != 0) {
        u16 ime;

        nxt = dest + TILE_BYTES;
        ncol = col + 1;
        pairs = sub >> 1;
        dst = dest + pairs * 2;
        for (i = TILE_ROWS - 1; i >= 0; i--) {
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = &fill;
            REG_DMA3.dst = dst;
            REG_DMA3.control = ((TILE_WIDTH - sub) / 2) | DMA_FILL_HALFWORDS;
            REG_DMA3.control;
            REG_IME = ime;
            dst += TILE_WIDTH;
        }
        dest = nxt;
        col = ncol;
    }

    if (col <= LAST_TILE_COL) {
        u16 ime;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        REG_DMA3.src = &fill;
        REG_DMA3.dst = dest;
        control = (((TILE_COLUMNS - col) << TILE_BYTE_SHIFT) >> 1)
                | DMA_FILL_HALFWORDS;
        REG_DMA3.control = control;
        REG_DMA3.control;
        REG_IME = ime;
    }
}
