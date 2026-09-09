/* Script command: hand the owner and an operand to FUN_08036338 — 0x0805A248-0x0805A273
 *
 * Answers 0 when the first operand resolves to nothing OR when the actor it
 * resolves to has no owner at +0x28; the sibling at 0x0805A220 treats the
 * missing owner as success instead.
 *
 * Rule 49: the single zero answer is at the end and both tests branch forward
 * to it.
 *
 * BOTH operands are u16 parameters, and that is what fixes the order of the two
 * normalisations at the top: agbcc emits them in parameter order, the first
 * into r0 and the second into r4, which is where each one's register allocation
 * puts it. Declaring the first operand u32 and letting the callee's u16
 * parameter convert it produces the same five instructions in the other order.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_owner_call_arg.c
 */

#include "gba_types.h"
#include "../script/script_actor.h"

extern ScriptActor *ResolveRecordNodeChain(u16 id);

extern void FUN_08036338(ScriptOwner *owner, u16 value);

/* 0x0805A248 */
u32 FUN_0805a248(u32 a, u16 id, u16 value)
{
    ScriptActor *actor = ResolveRecordNodeChain(id);
    ScriptOwner *owner;

    if (actor == 0) goto no;
    owner = actor->owner;
    if (owner == 0) goto no;
    FUN_08036338(owner, value);
    return 1;
no:
    return 0;
}
