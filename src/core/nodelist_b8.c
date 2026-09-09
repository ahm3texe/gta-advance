/* Progress condition query — 0x080552D4-0x080554BF (492 bytes)
 *
 * A SINGLE-PARAMETER PREDICATE: it takes a condition id between 0 and 23 and
 * returns 0 or 1. The ROM has a 24-entry JUMP TABLE
 * (0x080552EC-0x0805534B, 96 bytes), so the source is a dense `switch`:
 *
 *   push {lr} / cmp r0,#23 / bls .L / b default / lsls r0,#2 /
 *   ldr r1,=0x080552EC / adds r0,r0,r1 / ldr r0,[r0] / mov pc,r0
 *
 * The `bls .L + b default` pair after `cmp #23` is not something written by
 * hand: the default block (0x080554BA) is 482 bytes away and a Thumb
 * conditional branch does not reach beyond +-254 bytes, so agbcc produces its
 * own trampoline. For the same reason most of the bodies have `bls <near>` +
 * `b <far>` pairs.
 *
 * Table entry 0 -> default (0x080554BA), so THERE IS NO `case 0` IN THE
 * SOURCE.
 * Table entry 23 -> the shared `return 0` block, so `case 23: return 0;` has
 * been welded into the tail by cross-jumping.
 *
 * THE DATA SOURCE: gSaveBuffer (0x02000D50, data/ram_map.csv). Three separate
 * bitfield containers are read; the widths were computed BACKWARDS from the
 * extraction instructions (`(v << (32-bitpos-w)) >> (32-w)` is gcc's
 * extract_bit_field pattern):
 *
 *   +0x70  u32 container
 *       ldrb [+0x71] / lsls #27 / lsrs #27   -> bit  8, width 5
 *       ldr  [+0x70] / lsls #14 / lsrs #27   -> bit 13, width 5
 *       ldrb [+0x72] / lsls #25 / lsrs #27   -> bit 18, width 5
 *   +0x7C  u16 container
 *       ldrb [+0x7C] / lsls #28 / lsrs #28   -> bit  0, width 4
 *       ldrh [+0x7C] / lsls #21 / lsrs #25   -> bit  4, width 7
 *   +0x7E  u16 container
 *       ldrb [+0x7E] / lsls #26 / lsrs #27   -> bit  1, width 5
 *       ldrh [+0x7E] / lsls #21 / lsrs #27   -> bit  6, width 5
 *       ldrb [+0x7F] / lsrs #3               -> bit 11, width 5
 *
 * The single `lsrs #3` on the last line: because bits 11..15 end at the top of
 * the byte, gcc drops the mask and leaves only the shift. So the field is 5
 * bits.
 *
 * THE CONTAINER WIDTH IS MANDATORY: the 6..10 field at +0x7E CROSSES A BYTE
 * BOUNDARY (it is read with ldrh), so the bitfield type CANNOT be `u8`; it
 * must be `u16`. Likewise the 13..17 field at +0x70 is read with ldr -> `u32`.
 * +0x7C and +0x7E are SEPARATE containers: written contiguously, 5 bits would
 * be left empty in the 0x7C container and 0x7E's first field would shift into
 * them.
 *
 * MEASUREMENT LOG -- two rounds, both single-variable (whoever tries something
 * new should ADD TO THIS LIST and NOT DELETE existing lines):
 *
 * 1) WITHOUT `case 0`, 488/492 -- FOUR BYTES SHORT.
 *    The symptom: our prologue produced `subs r0,#1 / cmp r0,#22` and a
 *    23-entry table, whereas the ROM has `cmp r0,#23` and a 24-entry table.
 *    agbcc subtracts a switch's smallest case; with NO `subs`, the smallest
 *    case IS ZERO. So the source DOES have a `case 0:`, and because its body
 *    is the same as default's (`return 1`) it has been welded in by
 *    cross-jumping, leaving only an extra word in the table. Adding
 *    `case 0: return 1;` took 488 -> 492 (154 differences).
 *
 * 2) THE +0x70 CONTAINER'S COMPARISONS MUST BE UNSIGNED: 154 -> 0.
 *    The measured agbcc behavior (confirmed with a probe):
 *        a u16 field : 5   ->  `cmp #19 / bhi`   (UNSIGNED)
 *        a u32 field : 5   ->  `cmp #19 / bgt`   (SIGNED)
 *    So the bitfield's DECLARED TYPE decides the signedness of the comparison.
 *    Because the +0x7C and +0x7E containers are `u16`, they already gave the
 *    ROM's `bhi/bls`; the +0x70 container MUST BE `u32` (its bit 13..17 field
 *    crosses a halfword boundary, and declared `u16` agbcc would shift the
 *    field into the next u16 and the offset would move to +0x72), so the
 *    signedness was corrected in the source: a `U` suffix on the constants.
 *    Equivalents eliminated (all three give the SAME code and are redundant):
 *        (u32)gSaveBuffer.score1 > 9      -- an explicit cast
 *        u32 v = gSaveBuffer.score1; v>9  -- an intermediate local
 *        gSaveBuffer.score1 >= 10U        -- the >= form
 *
 *    This correction had a CHAIN effect, not just two instructions:
 *      - cases 3/5/7 could only cross-jump into case 1's `cmp #19` /
 *        case 2's `cmp #9` tail once the signedness matched (these are
 *        `b 0x8055372` and `b 0x8055388` in the ROM),
 *      - conversely cases 11..20 (the FUN_08030390 return, a SIGNED `bgt`) had
 *        been welded into those tails BY MISTAKE; once the signs separated
 *        they got their own `cmp/bgt` pairs back, as in the ROM.
 *    Thanks to the blocks that a single type decision both merged and
 *    separated, all 154 bytes closed in one move.
 *
 * MATCH: 492/492 bytes.
 *
 * AN EXTERNAL SYMBOL: 0x08061EC0 was MISSING from the map (the previous record
 *   is 0x08061DF0 and the next 0x08061EC4); in the ROM there is a 4-byte
 *   `movs r0,#1 / bx lr` there -- a real leaf function that Ghidra missed
 *   because it sits between two pool words. It was reported and added to
 *   data/functions.csv as `0x08061EC0,FUN_08061ec0,4`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b8.c
 */

#include "gba_types.h"

/* The face of the save buffer as seen from this translation unit. The other
 * views: src/world/area_flags.c (SaveBuffer) and src/world/stat_counters.c
 * (SaveCounters). Each TU declares its own local view. */
typedef struct SaveProgress {
    u8  pad00[0x70];            /* 0x00 */

    u32 unk70_0  : 8;           /* 0x70 bit  0 */
    u32 score1   : 5;           /* 0x70 bit  8 */
    u32 score2   : 5;           /* 0x70 bit 13 */
    u32 score3   : 5;           /* 0x70 bit 18 */
    u32 unk70_23 : 9;           /* 0x70 bit 23 */

    u8  pad74[8];               /* 0x74 */

    u16 tally0   : 4;           /* 0x7C bit  0 */
    u16 tally4   : 7;           /* 0x7C bit  4 */
    u16 unk7C_11 : 5;           /* 0x7C bit 11 */

    u16 unk7E_0  : 1;           /* 0x7E bit  0 */
    u16 lap1     : 5;           /* 0x7E bit  1 */
    u16 lap2     : 5;           /* 0x7E bit  6 */
    u16 lap3     : 5;           /* 0x7E bit 11 */
} SaveProgress;

extern SaveProgress gSaveBuffer;

extern s32 FUN_08030390(s32 a, s32 b);
extern s32 FUN_08061ec0(s32 a);

/* 0x080552D4 */
u32 IsProgressThresholdMet(u32 kind)
{
    switch (kind) {
    case 0:
        return 1;
    case 1:
        if (gSaveBuffer.lap1 > 19 && gSaveBuffer.lap2 > 19
            && gSaveBuffer.lap3 > 19)
            return 1;
        return 0;
    case 2:
        if (gSaveBuffer.score1 > 9U)
            return 1;
        return 0;
    case 3:
        if (gSaveBuffer.score1 > 19U)
            return 1;
        return 0;
    case 4:
        if (gSaveBuffer.score2 > 9U)
            return 1;
        return 0;
    case 5:
        if (gSaveBuffer.score2 > 19U)
            return 1;
        return 0;
    case 6:
        if (gSaveBuffer.score3 > 9U)
            return 1;
        return 0;
    case 7:
        if (gSaveBuffer.score3 > 19U)
            return 1;
        return 0;
    case 8:
        if (gSaveBuffer.tally0 > 7)
            return 1;
        return 0;
    case 9:
        if (gSaveBuffer.tally0 > 9)
            return 1;
        return 0;
    case 10:
        if (gSaveBuffer.tally4 > 49)
            return 1;
        return 0;
    case 11:
        if (FUN_08030390(79, 0) > 9)
            return 1;
        return 0;
    case 12:
        if (FUN_08030390(79, 0) > 19)
            return 1;
        return 0;
    case 13:
        if (FUN_08030390(79, 0) > 29)
            return 1;
        return 0;
    case 14:
        if (FUN_08030390(79, 0) > 39)
            return 1;
        return 0;
    case 15:
        if (FUN_08030390(79, 0) > 49)
            return 1;
        return 0;
    case 16:
        if (FUN_08030390(79, 0) > 59)
            return 1;
        return 0;
    case 17:
        if (FUN_08030390(79, 0) > 69)
            return 1;
        return 0;
    case 18:
        if (FUN_08030390(79, 0) > 79)
            return 1;
        return 0;
    case 19:
        if (FUN_08030390(79, 0) > 89)
            return 1;
        return 0;
    case 20:
        if (FUN_08030390(79, 0) > 99)
            return 1;
        return 0;
    case 21:
        if (FUN_08061ec0(2) != 0)
            return 1;
        return 0;
    case 22:
        if (FUN_08061ec0(3) != 0)
            return 1;
        return 0;
    case 23:
        return 0;
    }

    return 1;
}
