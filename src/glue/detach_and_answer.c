/* Detach the +0x24 handle, then answer 0 — 0x0804FFA4-0x0804FFB1
 *
 * Two wrappers over the actor helpers in src/actor/, each passing the +0x24
 * field on. They differ in the callee and in the answer: 0 for the detach, 1
 * for the arm.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value, so the constant
 * after the call is the answer and not a leftover.
 *
 * They are NOT adjacent in the ROM, so they cannot share a file; this one holds
 * the first and src/glue/arm_and_answer.c the second.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/detach_and_answer.c
 */

#include "gba_types.h"

typedef struct HandleOwner {
    u8   pad00[0x24];
    u32  handle;                /* +0x24 */
} HandleOwner;

extern void FUN_0803f7d4(u32 handle);

/* 0x0804FFA4 */
u32 FUN_0804ffa4(HandleOwner *owner)
{
    FUN_0803f7d4(owner->handle);
    return 0;
}
