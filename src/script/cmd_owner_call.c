/* Script command: hand the owner to FUN_080362E4 — 0x0805A274-0x0805A295
 *
 * The same shape as the sibling at 0x0805A248 without the second operand.
 *
 * Rule 49: the single zero answer is at the end and both tests branch forward
 * to it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_owner_call.c
 */

#include "gba_types.h"
#include "../script/script_actor.h"

extern ScriptActor *ResolveRecordNodeChain(u16 id);

extern void FUN_080362e4(ScriptOwner *owner);

/* 0x0805A274 */
u32 FUN_0805a274(u32 a, u32 id)
{
    ScriptActor *actor = ResolveRecordNodeChain(id);
    ScriptOwner *owner;

    if (actor == 0) goto no;
    owner = actor->owner;
    if (owner == 0) goto no;
    FUN_080362e4(owner);
    return 1;
no:
    return 0;
}
