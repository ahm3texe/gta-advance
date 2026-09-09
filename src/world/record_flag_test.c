/* Is flag 0x400 set on the owner's record — 0x0803FACC-0x0803FAE5
 *
 * Answers 0 when the owner has no record at +0x2C, or when bit 0x400 is clear
 * in the record's +0x1E halfword; 1 otherwise. The placeholder name is kept:
 * the callers were not examined, so what the flag means is not recorded.
 *
 * Rule 33: the ROM builds the mask FIRST (`movs r0,#128 / lsls r0,#3`) and
 * ands the field INTO the mask's register. Written as `field & 0x400` the
 * result stays in the field's register instead.
 *
 * Rule 49: the two `return 0` paths share one body at the end; `return 1` is
 * the one that falls through with a branch over it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/record_flag_test.c
 */

#include "gba_types.h"

#define RECORD_FLAG  (0x80 << 3)

typedef struct FlagRecord {
    u8  pad00[0x1E];
    u16 flags;                  /* +0x1E */
} FlagRecord;

typedef struct FlagOwner {
    u8          pad00[0x2C];
    FlagRecord *record;         /* +0x2C */
} FlagOwner;

/* 0x0803FACC */
u32 FUN_0803facc(FlagOwner *owner)
{
    FlagRecord *record;
    u32 mask;

    record = owner->record;
    if (record == 0) goto no;
    mask = RECORD_FLAG;
    mask &= record->flags;
    if (mask == 0) goto no;
    return 1;
no:
    return 0;
}
