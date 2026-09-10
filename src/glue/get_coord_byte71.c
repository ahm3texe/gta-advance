/* Read the coord block's +0x71 byte — 0x0800AB40-0x0800AB4B
 *
 * The offset is 113, past Thumb's ldrb immediate limit of 31, so the ROM adds
 * it to the base first. The CoordBlock body is copied from
 * src/misc/coord_more.c, which must keep the same layout.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/get_coord_byte71.c
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

/* 0x0800AB40 */
u32 FUN_0800ab40(void)
{
    return gRam02011030.byte71;
}
