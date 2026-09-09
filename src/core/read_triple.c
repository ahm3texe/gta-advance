/* Write three fields out — 0x0800A930-0x0800A94B
 *
 * It writes the +0x30, +0x34 and +0x38 fields of the 0x02011030 block into the
 * three pointers supplied by the caller.
 *
 * Rule 35: the final `pop {r0}; bx r0` says the return type is void (with a
 * u32 return, r0 would stay live and agbcc would take the return address into
 * r1).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/read_triple.c
 */

#include "gba_types.h"

/* THE DEFINITION must be kept BYTE-FOR-BYTE the same as in
   src/misc/coord_accessors.c and coord_more.c; a contradictory extern type for
   the same symbol breaks `make check` (TYPES-001). The fields were carved out
   of the padding; THE LAYOUT DID NOT CHANGE. */
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

/* 0x0800A930 */
void ReadTriple(u32 *outA, u32 *outB, u32 *outC)
{
    *outA = gRam02011030.a;
    *outB = gRam02011030.b;
    *outC = gRam02011030.c;
}
