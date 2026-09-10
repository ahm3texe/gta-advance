/* Answer the object itself when it is free and flagged — 0x08023754-0x08023777
 *
 * NOT BYTE-MATCHING. 12 of 17 instructions, two bytes: the ROM saves the
 * argument into a callee-saved register and then copies it BACK into r0 for the
 * call, and agbcc leaves it where it already is. Four spellings were measured,
 * including rule 75's mixed-type forms in both directions and a same-type
 * local, and the copy is propagated away in all of them. Register-copy class,
 * rule 44.
 *
 * All three failures share one `return 0` at the end, reached by forward
 * branches, and the success answers the argument.
 *
 * Rule 49 for the layout, which IS reproduced.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/self_if_free_and_flagged.c
 */

#include "gba_types.h"

typedef struct FlagOwner {
    u8 pad00[9];
    u8 flag;                    /* +0x09 */
} FlagOwner;

extern s32 GetOwnerSlot(FlagOwner *owner);

/* 0x08023754 */
FlagOwner *FUN_08023754(FlagOwner *owner)
{
    if (owner == 0) goto none;
    if (GetOwnerSlot(owner) != 0) goto none;
    if (owner->flag == 0) goto none;
    return owner;
none:
    return 0;
}
