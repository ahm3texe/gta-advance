/* Rewrite two priority fields — 0x080150D0-0x0801510F
 *
 * NOT BYTE-MATCHING. 14 of 30 instructions. The mask's own copy comes out
 * right (rule 33), but the ROM copies each OR-constant out of its build
 * register as well (`movs r4,#128 / lsls r4,#4 / adds r2,r4,#0`) and agbcc ors
 * directly from the build register; the record's pointer then lands in r2
 * instead of r3 and everything after shifts. Two spellings were measured, one
 * local for each constant and two, and neither produces the copy. This is the
 * register-copy class of rule 44, in the variant rule 75's mixed-type lever
 * does not reach.
 *
 * Runs FUN_080148B0 first, then, only when the +0x27 byte is exactly 1,
 * replaces bits 0x0C00 of the +0x18 record's +0x00 and +0x04 halfwords with
 * 0x800 and 0xC00.
 *
 * Rule 33 throughout: the mask 0xF3FF is copied out of its register before the
 * field is anded into it, and each of the two or-constants is copied as well.
 * The mask is loaded ONCE and used for both halfwords, which is what a single
 * local gives.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/set_two_priorities.c
 */

#include "gba_types.h"

#define KEEP_MASK   0xF3FF
#define FIRST_BITS  (128 << 4)  /* 0x0800 */
#define SECOND_BITS (192 << 4)  /* 0x0C00 */

typedef struct PriorityRecord {
    u16 first;                  /* +0x00 */
    u16 pad02;
    u16 second;                 /* +0x04 */
} PriorityRecord;

typedef struct PriorityOwner {
    u8               pad00[0x18];
    PriorityRecord  *record;    /* +0x18 */
    u8               pad1C[11];
    u8               state;     /* +0x27 */
} PriorityOwner;

extern void FUN_080148b0(PriorityOwner *owner);

/* 0x080150D0 */
void FUN_080150d0(PriorityOwner *owner)
{
    PriorityRecord *record;
    u16 mask;
    u16 value;
    u16 bits;
    u16 extra;

    FUN_080148b0(owner);
    if (owner->state != 1)
        return;
    record = owner->record;
    mask = KEEP_MASK;
    value = mask;
    value &= record->first;
    bits = FIRST_BITS;
    extra = bits;
    value |= extra;
    record->first = value;
    mask &= record->second;
    bits = SECOND_BITS;
    extra = bits;
    mask |= extra;
    record->second = mask;
}
