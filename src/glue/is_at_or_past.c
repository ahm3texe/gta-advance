/* Is the +0x38 field at or past the other's +0x08 — 0x0800C124-0x0800C133
 *
 * The comparison is SIGNED (`blt`), so both fields are ints.
 *
 * Rule 73: the 1 is written after the label, so it is the one that falls
 * through first -- the mirror of src/glue/is_coord_second_equal.c, whose ROM
 * has the two answers the other way round and whose source therefore has the
 * `goto` arms swapped.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/is_at_or_past.c
 */

#include "gba_types.h"

typedef struct Progress {
    u8  pad00[0x38];
    s32 reached;                /* +0x38 */
} Progress;

typedef struct Target {
    u8  pad00[8];
    s32 needed;                 /* +0x08 */
} Target;

/* 0x0800C124 */
u32 FUN_0800c124(const Progress *progress, const Target *target)
{
    if (progress->reached >= target->needed) goto yes;
    return 0;
yes:
    return 1;
}
