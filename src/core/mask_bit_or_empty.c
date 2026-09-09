/* Is the mask bit set, or is there no mask at all — 0x08051094-0x080510B1
 *
 * Three gates, all of which answer 1: a null object, a zero mask word at the
 * +0x2C record's +0x1C, or the requested bit already set in that mask. Only a
 * non-zero mask with the bit clear answers 0.
 *
 * The name states the test rather than its purpose: the callers were not
 * examined, so what the answer is used for is not recorded here. The three
 * early answers share one `movs r0,#1` at the end of the function, which is
 * rule 49 -- written as three separate `return 1`s the bodies come inline and
 * the block order inverts.
 *
 * Rule 33 decides the bit test. Written as `(1 << bit) & mask`, agbcc
 * canonicalizes it into `(mask >> bit) & 1` and emits `lsrs r2,r1 / movs r0,#1
 * / ands r2,r0`; the ROM shifts the CONSTANT and ands into its register.
 * Building the probe in its own local and updating it in place gives the ROM's
 * `movs r0,#1 / lsls r0,r1 / ands r0,r2`. Swapping the operands alone does not
 * do it: 2 instructions still differ.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/mask_bit_or_empty.c
 */

#include "gba_types.h"

typedef struct MaskRecord {
    u8  pad00[0x1C];
    u32 mask;                   /* +0x1C */
} MaskRecord;

typedef struct MaskOwner {
    u8          pad00[0x2C];
    MaskRecord *record;         /* +0x2C */
} MaskOwner;

/* 0x08051094 */
u32 FUN_08051094(MaskOwner *owner, u32 bit)
{
    u32 mask;
    u32 probe;

    if (owner == 0) goto yes;
    mask = owner->record->mask;
    if (mask == 0) goto yes;
    probe = 1;
    probe <<= bit;
    probe &= mask;
    if (probe != 0) goto yes;
    return 0;
yes:
    return 1;
}
