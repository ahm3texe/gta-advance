/* The percentage score from the link report — 0x08067014-0x08067183
 *
 * Runs a series of threshold checks over the fields of the report record,
 * counts how many hold, then returns the percentage
 * `(100 * (base + count)) / (baseLow + 23)`.  The result is clipped at 99; if
 * the threshold has already been exceeded it returns 100 directly.
 *
 * The statistics block is the 36 bytes copied at 0x08066AA8 from the record
 * buffer's +0x64.  The 5-bit counters inside it have the same layout as in
 * BumpRankCounter (0x08066C94): the report's +0x2E is gSaveBuffer's +0x7E
 * itself.
 *
 * The division is written as a direct __divsi3 call; the `/` operator produces
 * a different helper (measured in src/text/text_f5.c).
 *
 * STATUS: PARKED — 332/368 bytes, 85/183 instructions the same.  Its structure
 * and threshold chain are right; the remaining difference is register
 * allocation.
 *
 * TWO CLASSES CLOSED IN THE LAST ROUND:
 *   - `rankA/B/C > 19` must be UNSIGNED (`> (u32)19`).  Even though the field
 *     is `u32 : 5`, it promotes to `int`, so a plain `> 19` produced a SIGNED
 *     `ble`; the ROM has `bls` (rule 56's saturation row).  lapA/B/C already
 *     produce `bls` with the same spelling and need no cast.
 *   - THE ORDER of the tail block: `if (total >= limit) return 100;` then the
 *     plain computation.  The earlier
 *     `if (total < limit) { computation } return 100;` put the computation
 *     first; the ROM places the 100 return first
 *     (`blt <computation> / movs r0,#100 / b <end>`).  The tail is now exactly
 *     the same.
 *
 * THE ONE REMAINING CLASS (36 bytes = ~18 instructions): the ROM keeps the
 * `report` pointer in `ip` (r12) and copies it into a low register before
 * every access (`mov r0, ip`); we keep it in r3 and never emit those 15 copy
 * instructions.  The reason shows up in the allocation table (dump_alloc):
 *   - In the lap triple we, like the ROM, keep the INTERMEDIATE `<<` result
 *     live (109/114/121 -> r6/r5/r4) and re-emit the `lsrs` in the sums; that
 *     block is identical to the ROM's.
 *   - In the rank triple, however, CSE merges the WHOLE `(x<<k)>>27`
 *     expression and keeps the EXTRACTED value live (95/98/103 -> r12/r8/r1),
 *     reducing both sum tests to a single computation; the ROM recomputes both.
 *     The difference comes from the container width: the lap fields are in a
 *     `u16` container (the HImode intermediate conversions break CSE) while the
 *     rank fields are in a `u32` one.  Because rankB spans bits 13-17 the
 *     container MUST be `u32` (rule 61; the ROM does `ldr r0,[r1,#32]`), so
 *     this lever cannot be turned from the source side.  With the low registers
 *     freed up, `report` stays in r3 and the ROM's `ip` form does not appear.
 *
 * THE LEVER FOUND (applied): if you use a `u8` field in more than one
 * expression, take it into a LOCAL FIRST.  Direct member access makes agbcc
 * emit a redundant `lsls #24 / lsrs #24` zero-extension pair; a local removes
 * it.  Measured on the queryA pair.
 *
 * SPELLINGS RULED OUT (measured this round):
 *   - Applying the same lever to the queryB quadruple (curB/altB locals):
 *     85 -> 59 instructions.  Because the ROM RE-READS +0x06 and +0x07, direct
 *     member access is required there -- the reverse direction of rule 55,
 *     confirmed.
 *   - Removing the `(s32)` cast from the sum tests: no change (85).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_score.c
 */

#include "gba_types.h"

#define SCORE_FULL   100
#define SCORE_CAP    99
#define LIMIT_BONUS  23

typedef struct ScoreStats {
    u8  pad00[0x0C];
    u32 lowByte : 8;            /* +0x0C bit 0-7 */
    u32 rankA   : 5;            /* bit 8-12 */
    u32 rankB   : 5;            /* bit 13-17 */
    u32 rankC   : 5;            /* bit 18-22 */
    u32 highBits: 9;            /* bit 23-31 */
    u8  pad10[0x18 - 0x10];
    u16 lowNib  : 4;            /* +0x18 bit 0-3 */
    u16 distance: 7;            /* bit 4-10 */
    u16 highPad : 5;            /* bit 11-15 */
    u16 spare   : 1;            /* +0x1A bit 0 */
    u16 lapA    : 5;            /* bit 1-5 */
    u16 lapB    : 5;            /* bit 6-10 */
    u16 lapC    : 5;            /* bit 11-15 */
    u8  pad1C[0x24 - 0x1C];
} ScoreStats;

typedef struct ScoreReport {
    u16        head;            /* +0x00 */
    u16        headAlt;         /* +0x02 */
    u8         queryA;          /* +0x04 */
    u8         queryAAlt;       /* +0x05 */
    u8         queryB;          /* +0x06 */
    u8         queryBAlt;       /* +0x07 */
    u8         g5v1;            /* +0x08 */
    u8         g5v1Alt;         /* +0x09 */
    u8         g5v2;            /* +0x0A */
    u8         g5v2Alt;         /* +0x0B */
    u8         g5v3;            /* +0x0C */
    u8         g5v3Alt;         /* +0x0D */
    u8         summary;         /* +0x0E */
    u8         g7v2;            /* +0x0F */
    u8         g7v2Alt;         /* +0x10 */
    u8         pad11[3];
    ScoreStats stats;           /* +0x14 */
} ScoreReport;

typedef struct SaveNibble {
    u8 pad00[0x7C];
    u8 nibble : 4;              /* +0x7C bit 0-3 */
    u8 rest   : 4;
} SaveNibble;

extern SaveNibble gSaveBuffer;
extern s32 __divsi3(s32 dividend, s32 divisor);

/* 0x08067014 */
s32 FUN_08067014(ScoreReport *report)
{
    s32 count;
    s32 limit;
    s32 total;
    s32 result;
    u32 curA;
    u32 altA;

    count = 0;
    if (report->stats.distance > 49)
        count = 1;
    if (report->stats.distance > 99)
        count++;

    altA = report->queryAAlt;
    curA = report->queryA;
    if (curA >= altA >> 1)
        count++;
    if (curA >= altA)
        count++;

    if (report->queryB >= report->queryBAlt)
        count++;
    if (report->queryB >= report->queryBAlt >> 2)
        count++;
    if (report->queryB >= report->queryBAlt >> 1)
        count++;
    if (report->queryB >= (s32)(report->queryBAlt * 3) >> 2)
        count++;

    if (report->g5v1 >= report->g5v1Alt)
        count++;
    if (report->g5v2 >= report->g5v2Alt)
        count++;
    if (report->g5v3 >= report->g5v3Alt)
        count++;

    if (report->stats.rankA > (u32)19)
        count++;
    if (report->stats.rankB > (u32)19)
        count++;
    if (report->stats.rankC > (u32)19)
        count++;

    if (report->stats.lapA > 19)
        count++;
    if (report->stats.lapB > 19)
        count++;
    if (report->stats.lapC > 19)
        count++;

    if ((s32)(report->stats.rankA + report->stats.rankB + report->stats.rankC) > 59)
        count++;
    if ((s32)(report->stats.rankA + report->stats.rankB + report->stats.rankC) > 29)
        count++;

    if ((s32)(report->stats.lapA + report->stats.lapB + report->stats.lapC) > 59)
        count++;
    if ((s32)(report->stats.lapA + report->stats.lapB + report->stats.lapC) > 29)
        count++;

    if (gSaveBuffer.nibble > 11)
        count++;

    if (report->g7v2 != 0)
        count++;

    limit = report->headAlt + LIMIT_BONUS;
    total = report->head + count;

    if (total >= limit)
        return SCORE_FULL;

    result = __divsi3(SCORE_FULL * total, limit);
    if (result > SCORE_CAP)
        result = SCORE_CAP;
    return result;
}
