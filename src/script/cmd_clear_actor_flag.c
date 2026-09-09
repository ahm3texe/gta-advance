/* Script command: clear flag 0x200 on the actor — 0x08059F64-0x08059F8B
 *
 * The counterpart of the sibling at 0x08059F40, which sets the same bit.
 * ~0x200 is 0xFFFFFDFF and reaches the code through the literal pool, which is
 * why this function is four bytes longer than the one that sets the bit.
 *
 * The zero answer stands FIRST in the ROM, with the branch jumping over it into
 * the body. That is what a two-armed if produces; an early `if (actor == 0)
 * return 0;` puts the body first instead and does not match.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_clear_actor_flag.c
 */

#include "gba_types.h"
#include "phase1_types.h"

#define ACTOR_FLAG  (128 << 2)

extern Phase1GroupedActor *ResolveRecordNodeChain(u16 id);

/* 0x08059F64 */
u32 FUN_08059f64(u32 a, u32 id)
{
    Phase1GroupedActor *actor = ResolveRecordNodeChain(id);

    u32 result;

    if (actor == 0) {
        result = 0;
    } else {
        actor->flags &= ~ACTOR_FLAG;
        result = 1;
    }
    return result;
}
