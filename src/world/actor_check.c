/* Actor state check and direction difference — 0x08019B08-0x08019B5B
 *
 * The first function checks the actor flags for availability: bit 8 takes the
 * special validation path (return 1); stop if bit 0 is clear, bit 11 is set,
 * or the sub-object's field at +48 is 2. Otherwise return 1.
 * The second extracts a 3-bit direction index from the difference returned
 * by FUN_08017D78.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_check.c
 */

#include "gba_types.h"

#define FLAG_SPECIAL   0x0080
#define FLAG_READY     0x0001
#define FLAG_BLOCK     0x0800

typedef struct Sub {
    u8 pad00[48];
    u8 state;                   /* +0x30 */
} Sub;

typedef struct Actor {
    u8   pad00[12];
    u32  flags;                 /* +0x0C */
    u8   pad10[8];
    Sub *sub;                   /* +0x18 */
} Actor;

extern s32 FlagsToAngle(u32 arg);

/* 0x08019B08 */
u32 IsActorUsable(Actor *a)
{
    u32 flags;

    if (a == 0)
        return 0;

    flags = a->flags;
    if ((flags & FLAG_SPECIAL) != 0)
        return 1;
    if ((flags & FLAG_READY) == 0)
        return 0;
    if ((flags & FLAG_BLOCK) != 0)
        return 0;
    if (a->sub->state == 2)
        return 0;

    return 1;
}

/* 0x08019B3C */
s32 GetAngleField(u32 base, u32 arg)
{
    s32 diff;

    diff = FlagsToAngle(arg);
    return ((diff - (s32)base + 0x800000) >> 24) & 3;
}
