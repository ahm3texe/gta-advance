/* Script command: set flag 0x200 on the actor — 0x08059F40-0x08059F63
 *
 * Resolves the operand to an actor and sets bit 9 of its +0x18 flags, answering
 * 1 on success and 0 when the operand resolves to nothing. The sibling at
 * 0x08059F64 clears the same bit.
 *
 * 0x200 is built as `movs r1,#128 / lsls r1,#2` because Thumb's movs immediate
 * stops at 255; the clearing sibling needs ~0x200, which has no such form at
 * all and goes through the literal pool instead.
 *
 * The zero answer stands FIRST in the ROM, with the branch jumping over it into
 * the body. That is what a two-armed if produces; an early `if (actor == 0)
 * return 0;` puts the body first instead and does not match.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_set_actor_flag.c
 */

#include "gba_types.h"
#include "phase1_types.h"

#define ACTOR_FLAG  (128 << 2)

extern Phase1GroupedActor *ResolveRecordNodeChain(u16 id);

/* 0x08059F40 */
u32 FUN_08059f40(u32 a, u32 id)
{
    Phase1GroupedActor *actor = ResolveRecordNodeChain(id);

    u32 result;

    if (actor == 0) {
        result = 0;
    } else {
        actor->flags |= ACTOR_FLAG;
        result = 1;
    }
    return result;
}
