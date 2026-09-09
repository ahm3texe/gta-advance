/* Script command: set flag 0x200 on the actor's owner — 0x0805A220-0x0805A247
 *
 * Resolves the operand to an actor and sets bit 9 of the +0x0C flags of the
 * owner at +0x28. An actor with no owner is not an error: the handler still
 * answers 1, and only an operand that resolves to nothing answers 0.
 *
 * Rule 71: the zero answer stands FIRST in the ROM, with the branch jumping
 * over it into the body, so it is the `then` arm of a two-armed if.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_set_owner_flag.c
 */

#include "gba_types.h"
#include "../script/script_actor.h"

#define OWNER_FLAG  (128 << 2)

extern ScriptActor *ResolveRecordNodeChain(u16 id);

/* 0x0805A220 */
u32 FUN_0805a220(u32 a, u32 id)
{
    ScriptActor *actor = ResolveRecordNodeChain(id);
    ScriptOwner *owner;
    u32 result;

    if (actor == 0) {
        result = 0;
    } else {
        owner = actor->owner;
        if (owner != 0)
            owner->flags |= OWNER_FLAG;
        result = 1;
    }
    return result;
}
