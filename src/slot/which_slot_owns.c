/* Which slot owns this object — 0x0803BFE4-0x0803C043
 *
 * Answers 1 when the object is the first slot's own or the owner behind
 * gRam020272C8, 2 when it is the second slot's or the owner behind
 * gRam02026F34, and 0 otherwise. The second half only runs while byte 12 of
 * gGameState is set, which is what data/ram_map.csv already records as
 * gRam02026F34's validity condition.
 *
 * The 2 is materialised between the last field READ and the comparison that
 * uses it, so the read goes into a local of its own and the result is assigned
 * after it. Written as `if (owner->object == object)` the constant comes first
 * and the load second.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/which_slot_owns.c
 */

#include "gba_types.h"

#define TWO_PLAYER  12          /* gGameState byte 12 */

typedef struct SlotOwner {
    u8    pad00[0x28];
    void *object;               /* +0x28 */
} SlotOwner;

extern u8   gRam02000F10[];
extern u32  gRam020272C8;
extern u8   gGameState[];
extern u8   gRam02001140[];
extern u32  gRam02026F34;

/* 0x0803BFE4 */
u32 FUN_0803bfe4(void *object)
{
    SlotOwner *owner;
    void *held;
    u32 result;

    if (object == 0) goto none;
    if (*(void **)gRam02000F10 == object) goto first;
    owner = (SlotOwner *)gRam020272C8;
    if (owner == 0) goto second;
    if (owner->object != object) goto second;
first:
    result = 1;
    goto done;
second:
    if (gGameState[TWO_PLAYER] == 0) goto none;
    if (*(void **)gRam02001140 == object) {
        result = 2;
        goto done;
    }
    owner = (SlotOwner *)gRam02026F34;
    if (owner == 0) goto none;
    held = owner->object;
    result = 2;
    if (held == object) goto done;
none:
    result = 0;
done:
    return result;
}
