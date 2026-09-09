/* Does the +0x09 byte resolve — 0x080380B4-0x080380C9
 *
 * FUN_0805CFCC answers a signed value, and a negative one means "no". The test
 * is `blt`, so the answer is signed even though this function's own is not.
 *
 * The `goto` form, and note which way round: the body written AFTER the label
 * is the one that lands FIRST in the ROM, reached by falling through, as
 * src/script/cmd_area_ready.c also records. A two-armed if with a result
 * variable does not work here at all -- agbcc turns it branchless
 * (`mvns / lsrs #31`) and loses the comparison entirely.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/is_lookup_nonnegative.c
 */

#include "gba_types.h"

typedef struct KindEntity {
    u8 pad00[9];
    u8 index;                   /* +0x09 */
} KindEntity;

extern s32 FUN_0805cfcc(u32 index);

/* 0x080380B4 */
u32 FUN_080380b4(KindEntity *entity)
{
    if (FUN_0805cfcc(entity->index) >= 0) goto yes;
    return 0;
yes:
    return 1;
}
