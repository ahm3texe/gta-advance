/* Is either field in range — 0x080195F4-0x0801961F
 *
 * Two gates testing the ranges 12-13 and 15-16.  The first field is compared
 * SIGNED (blt/ble/bgt/bge), the second UNSIGNED (bcc/bls/bhi).
 *
 * THE FIELD MUST BE u16, NOT s16.  The ROM loads it UNSIGNED with `ldrh` yet
 * the comparisons are signed: that is C's natural behavior -- a u16 field is
 * promoted to `int` in a comparison.  Writing s16 produced `ldrsh` (2 bytes
 * too many).  The second field is u32, so its comparisons stay unsigned.
 *
 * MATCHED (44/44) -- see the MECHANISM note above the function.
 */

#include "gba_types.h"

typedef struct Entry {
    u8  pad00[6];
    u16 kind;                   /* +0x06 */
    u8  pad08[32];
    u32 alt;                    /* +0x28 (unsigned) */
} Entry;

/* 0x080195F4 */
/* MECHANISM (permuter + by hand, 2026-09-05): once the comparison constant is
 * taken into a VARIABLE, agbcc CANNOT CANONICALISE `x < 15` into `x <= 14`,
 * and it produces exactly the ROM's `cmp #15 / bcc` form.  No variety of the
 * literal spelling (< 15, <= 14, > 14 ...) gave this: they all collapse to the
 * same canonical form.  All three bounds were moved this way.
 * docs/COMPILER.md rule 44. */
u32 EitherInRange(Entry *entry)
{
    s32 lowBound;
    s32 highBound;
    s32 kind;
    u32 alt;
    u32 altHigh;

    kind = entry->kind;
    lowBound = 12;
    if (kind >= lowBound) {
        if (kind <= 13)
            goto yes;
        if (kind <= 16) {
            if (kind >= (highBound = 15))
                goto yes;
        }
    }
    alt = entry->alt;
    if (alt < lowBound)
        goto no;
    if (alt <= 13)
        goto yes;
    if (alt > 16)
        goto no;
    altHigh = 15;
    if (alt < altHigh)
        goto no;
yes:
    return 1;
no:
    return 0;
}
