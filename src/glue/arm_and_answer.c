/* Arm the +0x24 handle, then answer 1 — 0x080500C0-0x080500CD
 *
 * The sibling of src/glue/detach_and_answer.c; see that file for the shape.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/arm_and_answer.c
 */

#include "gba_types.h"

typedef struct HandleOwner {
    u8   pad00[0x24];
    u32  handle;                /* +0x24 */
} HandleOwner;

extern void FUN_0803f880(u32 handle);

/* 0x080500C0 */
u32 FUN_080500c0(HandleOwner *owner)
{
    FUN_0803f880(owner->handle);
    return 1;
}
