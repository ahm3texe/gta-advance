/* Historical band B: nine functions from 0x08030B34 .. 0x08031D23.
 *
 * MEASUREMENT NOTE: results from the former src/world/band_b.c were misleading.
 * agbcc_build.py links all functions consecutively from min(address), using
 * SUBALIGN(1). These nine are not contiguous in the ROM: other translation
 * units intervene. All but the first (SendTextMode1) were linked at a fixed
 * displacement from their ROM addresses, making every bl offset wrong even
 * for otherwise matching code (reported as 2/N differing bytes).
 * Each was therefore measured separately at its own ROM address:
 *   SendTextMode1 12 BYTE-MATCHING
 *   LoadHudPalettes 124 BYTE-MATCHING
 *   TriggerEvent39 12 BYTE-MATCHING
 *   GetRecordNodeById 14 BYTE-MATCHING
 *   ScaleMagnitude 132 BYTE-MATCHING
 *   ClearHudRowsAB 72 BYTE-MATCHING
 *   ReleaseActorAndSlot 64 BYTE-MATCHING
 *   BlitStripClipLeft4bpp 470 NON-MATCHING
 *   PushSlotQueueEntry 140 MISSING RAM SYMBOL
 * These are historical results; detailed notes accompany the split sources.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm (docs/COMPILER.md)
 * Verification: make c-match FILE=src/world/scale_magnitude.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* ---- 0x08031414 — 132 bytes, BYTE-MATCHING ------------------------------
 *
 * Maps a signed 16.16 fixed-point magnitude through a piecewise-linear curve.
 * The input's absolute value is taken and split into five bands; each band has
 * the form `((v - band_start) * slope >> 16) + base`, and the bands are
 * continuous at their boundaries (20 / 60 / 100 / 200).
 *
 * TWO MEASUREMENTS:
 *  - THE SPELLING OF THE ABSOLUTE VALUE.  The ROM emits
 *    `adds r1,r0,#0 / cmp r1,#0 / bge / negs r0,r1 / adds r1,r0,#0`: the
 *    negation's DESTINATION is the parameter's register, then it is copied
 *    back.  The plain `if (value < 0) value = -value; v = value;` gives a
 *    single `negs r1,r1` (71/132 bytes off).  The three-line form below --
 *    copy v first, write the negation INTO THE PARAMETER, refresh v -- gives
 *    the ROM's five-instruction sequence exactly.  Equivalents ruled out (all
 *    71 off): the ternary operator, `0 - value`, a single-variable spelling,
 *    two temporaries.
 *  - THE ORDER OF THE LAST TWO BANDS.
 *    `if (v > BAND4_END) return ...+200; return ...+100;` gets inverted by
 *    agbcc and the +100 body moves ahead (30 bytes off).  The ROM's `ble` plus
 *    body order comes out only with `if (v <= BAND4_END) return ...+100;` --
 *    that is, the SAME pattern as the first three bands.  Adding `else` has no
 *    effect.
 *  - The slopes are plain multiplications in the source: agbcc synthesises
 *    10/20/40 as `(v<<2)+v` plus a shift, and does 50 as
 *    `movs r0,#50 / muls r0,r1` (rule 53).
 */

#define FRAC_BITS  16
#define BAND1_END  0x20000
#define BAND2_END  0x40000
#define BAND3_END  0x50000
#define BAND4_END  0x70000
#define SLOPE1     10
#define SLOPE2     20
#define SLOPE3     40
#define SLOPE4     50
#define SLOPE5     40
#define BASE2      20
#define BASE3      60
#define BASE4      100
#define BASE5      200

/* 0x08031414 */
s32 ScaleMagnitude(s32 value)
{
    s32 v;

    v = value;
    if (v < 0)
        value = -v;
    v = value;

    if (v <= BAND1_END)
        return (v * SLOPE1) >> FRAC_BITS;
    if (v <= BAND2_END)
        return (((v - BAND1_END) * SLOPE2) >> FRAC_BITS) + BASE2;
    if (v <= BAND3_END)
        return (((v - BAND2_END) * SLOPE3) >> FRAC_BITS) + BASE3;
    if (v <= BAND4_END)
        return (((v - BAND3_END) * SLOPE4) >> FRAC_BITS) + BASE4;
    return (((v - BAND4_END) * SLOPE5) >> FRAC_BITS) + BASE5;
}

