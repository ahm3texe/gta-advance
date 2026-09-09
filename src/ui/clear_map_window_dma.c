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
 * Verification: make c-match FILE=src/ui/clear_map_window_dma.c
 */

#include "gba_io.h"
#include "gba_types.h"

#define TILE_BLANK   0xF0E8

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 text, u32 kind);

/* 0x08031328 — BYTE-MATCHING. Two DMA3 fills: 0x280 entries of empty tile
 * from 0x06009800, then 0x10 zero entries from 0x06009D00. The fixed source
 * is a single stack halfword. The ROM reads control back; omitting this
 * loses two bytes per block (COMPILER.md, DMA control readback). The stack
 * buffer must be volatile (rule 3).
 */
#define DMA_FILL16(n)  (0x81000000 | (n))
#define MAP_FILL_DST   ((void *)0x06009800)
#define MAP_ZERO_DST   ((void *)0x06009D00)

void ClearMapWindowDma(void)
{
    u16 ime;
    volatile u16 fill;

    ime = REG_IME;
    REG_IME = 0;
    fill = TILE_BLANK;
    REG_DMA3.src = (void *)&fill;
    REG_DMA3.dst = MAP_FILL_DST;
    REG_DMA3.control = DMA_FILL16(0x280);
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = (void *)&fill;
    REG_DMA3.dst = MAP_ZERO_DST;
    REG_DMA3.control = DMA_FILL16(0x10);
    REG_DMA3.control;
    REG_IME = ime;
}

