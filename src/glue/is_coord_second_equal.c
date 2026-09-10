/* Does the coord block's +0x04 match — 0x0800AB4C-0x0800AB5F
 *
 * Rule 73: the 1 is written AFTER the label, so the 0 falls through first,
 * which is the layout the ROM has. src/glue/is_record_kind_six.c is the same
 * shape over a byte field.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/is_coord_second_equal.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;              /* +0  */
    u32 unk04;              /* +4  */
    u32 unk08;              /* +8  */
    s32 second;             /* +12 */
    s32 first;              /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;              /* +72 */
    u8  pad4C[37];
    u8  byte71;             /* +0x71 */
} CoordBlock;

extern CoordBlock gRam02011030;

/* 0x0800AB4C */
u32 FUN_0800ab4c(u32 value)
{
    if (gRam02011030.unk04 != value) goto no;
    return 1;
no:
    return 0;
}
