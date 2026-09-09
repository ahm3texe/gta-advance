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
 * Verification: make c-match FILE=src/ui/clear_hud_field_c.c
 */

#include "gba_io.h"
#include "gba_types.h"

#define TILE_BLANK   0xF0E8

extern u32  GetTextString(u32 index);
extern void FUN_0802af40(u32 text, u32 kind);

/* 0x08030F50 — BYTE-MATCHING. Instruction-for-instruction identical to
 * ClearHudFieldB (0x08030F28) except for two VRAM columns. Uses the same
 * array-index loop (hud_fields.c, F28). Keep the tile constant directly in
 * the store; a local promotes it to SImode and removes the ROM's adds r3,r0,#0.
 */
#define COL_C_LEFT   ((vu16 *)0x06009952)
#define COL_C_RIGHT  ((vu16 *)0x06009992)

void ClearHudFieldC(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = COL_C_LEFT;
    b = COL_C_RIGHT;
    for (i = 0; i < 6; i++) {
        a[i] = TILE_BLANK;
        b[i] = TILE_BLANK;
    }
}

