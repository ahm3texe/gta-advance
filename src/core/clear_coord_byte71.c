/* Clear the byte at CoordBlock +0x71 — 0x0800AB30-0x0800AB3F
 *
 * The CoordBlock definition must be kept BYTE-FOR-BYTE the same as in
 * src/misc/coord_accessors.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/clear_coord_byte71.c
 */

#include "gba_types.h"

typedef struct CoordBlock {
    u32 unk00;                  /* +0  */
    u32 unk04;                  /* +4  */
    u32 unk08;                  /* +8  */
    s32 second;                 /* +12 */
    s32 first;                  /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;                  /* +72 */
    u8  pad4C[37];
    u8  byte71;                 /* +0x71 */
} CoordBlock;

extern CoordBlock gRam02011030;

/* 0x0800AB30 */
void ClearCoordByte71(void)
{
    gRam02011030.byte71 = 0;
}
