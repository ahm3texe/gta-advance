/* Scale the value by the owner's byte — 0x0801D990-0x0801D9B9
 *
 * With no owner the value comes back untouched; with an owner but no +0x2C
 * record, likewise. Otherwise it is multiplied by `(byte & 0xFF) + 128` and
 * shifted down 8, so a byte of 0 halves it and 128 leaves it alone.
 *
 * The shift down is ARITHMETIC, so the value is signed.
 *
 * Rule 33 for the mask, and rule 72 for where it goes: the 0xFF is materialised
 * AFTER the call, so the callee's answer goes into a local of its own first.
 * Written `factor = 0xFF; factor &= FUN_080325D0(...)` the constant is live
 * across the call and the function pays for a second callee-saved register.
 *
 * The two untouched paths are separate in the ROM: one branches to the shared
 * return and the other re-copies the value first, which is what an `if` nested
 * inside an `if` gives rather than a combined test.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/scale_by_owner_byte.c
 */

#include "gba_types.h"

#define BYTE_MASK  0xFF
#define HALF_ONE   128
#define FRAC_BITS  8

typedef struct ScaleRecord {
    u32 value;                  /* +0x00 */
} ScaleRecord;

typedef struct ScaleOwner {
    u8           pad00[0x2C];
    ScaleRecord *record;        /* +0x2C */
} ScaleOwner;

extern u32 FUN_080325d0(u32 value);

/* 0x0801D990 */
s32 FUN_0801d990(s32 value, ScaleOwner *owner)
{
    ScaleRecord *record;
    u32 raw;
    s32 factor;

    if (owner == 0)
        return value;
    record = owner->record;
    if (record == 0)
        return value;
    raw = FUN_080325d0(record->value);
    factor = BYTE_MASK;
    factor &= raw;
    factor += HALF_ONE;
    return value * factor >> FRAC_BITS;
}
