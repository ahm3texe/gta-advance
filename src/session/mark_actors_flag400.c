/* Set flag 0x400 down the actor list — 0x08062134-0x08062163
 *
 * Walks the list from GetUnk02028290 through the +0x00 link, and sets bit 10 of
 * each actor's +0x0C flags where a DIFFERENT mask, 0x00400010, is clear.
 *
 * The test mask and the bit set are not the same value, which is what needs the
 * two constants to be separate locals hoisted out of the loop. 0x00400010 comes
 * from the literal pool; 0x400 is built with a shift.
 *
 * Rule 33: the ROM copies the flags into another register and ands the mask
 * into THAT (`adds r0,r1,#0 / ands r0,r4`), leaving the flags intact for the
 * `orrs` that may follow.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/mark_actors_flag400.c
 */

#include "gba_types.h"

#define BLOCKING   0x00400010
#define MARK_FLAG  (128 << 3)

typedef struct ListActor {
    struct ListActor *next;     /* +0x00 */
    u8                pad04[8];
    u32               flags;    /* +0x0C */
} ListActor;

extern ListActor *GetUnk02028290(void);

/* 0x08062134 */
void FUN_08062134(void)
{
    ListActor *actor = GetUnk02028290();
    u32 blocking;
    u32 mark;
    u32 flags;
    u32 probe;

    if (actor == 0)
        return;
    blocking = BLOCKING;
    mark = MARK_FLAG;
    do {
        flags = actor->flags;
        probe = flags;
        probe &= blocking;
        if (probe == 0) {
            flags |= mark;
            actor->flags = flags;
        }
        actor = actor->next;
    } while (actor != 0);
}
