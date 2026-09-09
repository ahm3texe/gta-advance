/* Accumulating distance and the overflow counter — 0x080672B8-0x080672FB
 *
 * Adds the low 16 bits of the incoming value's absolute value to the
 * accumulator; if the accumulation exceeds 0x1FFF the overflowing part is
 * carried into the counter at gSaveBuffer +0x76 and the accumulator is
 * masked.  If the counter wraps, the old value is written back (saturation).
 *
 * The ROM RE-READS the counter AFTER writing it and compares; that is why the
 * read in the comparison goes through the volatile view.  The same solution
 * was measured in src/world/distance_accum.c: making the whole field volatile
 * is too strong and breaks the first read too, so only the second read must
 * be kept narrow.
 *
 * The SaveBuffer definition must be IDENTICAL to the one in
 * src/world/copy_flag_byte.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/accumulate_distance.c
 */

#include "gba_types.h"

#define ACC_MASK  0x1FFF
#define ACC_SHIFT 13

typedef struct Accum {
    u8  pad00[4];
    u32 value;                  /* +0x04 */
} Accum;

typedef struct SaveBuffer {
    u8 pad00[8];
    u8 byte8;                   /* +0x08 */
    u8 byte9;                   /* +0x09 */
    u8 pad0A[92];
    u16 counter66;              /* +0x66 */
    u8 pad68[14];
    u16 counter76;              /* +0x76 */
} SaveBuffer;

extern Accum      gDistanceAccum;
extern SaveBuffer gSaveBuffer;

/* 0x080672B8 */
void AccumulateDistance(s32 delta)
{
    Accum *accum;
    u32 acc;
    u16 old;

    /* The ROM loads the accumulator base BEFORE the absolute value
       computation (ldr r4 right at the top).  Writing gDistanceAccum
       directly moves the load
       to the point of use; a separate local pins the order down. */
    accum = &gDistanceAccum;

    if (delta < 0)
        delta = -delta;

    acc = accum->value + (delta >> 16);
    accum->value = acc;

    if (acc > ACC_MASK) {
        old = gSaveBuffer.counter76;
        gSaveBuffer.counter76 = old + (acc >> ACC_SHIFT);
        acc &= ACC_MASK;
        accum->value = acc;
        if (*(volatile u16 *)&gSaveBuffer.counter76 < old)
            gSaveBuffer.counter76 = old;
    }
}
