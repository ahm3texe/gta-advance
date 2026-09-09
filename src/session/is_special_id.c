/* Is the id one of six — 0x080623E4-0x08062409
 *
 * Answers 1 for 105, 106, 107, 113, 108 and 109. The ROM tests them in that
 * order, with 113 sitting between 107 and 108, so the source lists them the
 * same way; sorting them would reorder the comparisons.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * NOT BYTE-MATCHING. 10 of 19 instructions, and the difference is two bytes:
 * the ROM copies the narrowed id into a second register up front
 * (`adds r1,r0,#0`) and runs the LAST of the six comparisons against the copy.
 * Everything else, including the order of the six values, is reproduced; the
 * branch targets differ only because the missing copy shifts them.
 *
 * This is the register-copy class, the one docs/COMPILER.md records under
 * rule 44 as having no known source lever. Four spellings were measured:
 *
 *   six gotos to a shared `yes`                    10/19  (this one)
 *   a second local holding the id, used on the last test  10/19, copy still gone
 *   a `||` chain                                    5/19
 *   a `switch` with six labels                      6/19
 *   a result variable set to 1 first, cleared last   8/19
 *
 * agbcc propagates the copy away in every form that produces it. Left in place
 * as a record; functions.csv keeps it as decompiled, not matching.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/is_special_id.c
 */

#include "gba_types.h"

/* 0x080623E4 */
u32 FUN_080623e4(u16 id)
{
    if (id == 105) goto yes;
    if (id == 106) goto yes;
    if (id == 107) goto yes;
    if (id == 113) goto yes;
    if (id == 108) goto yes;
    if (id == 109) goto yes;
    return 0;
yes:
    return 1;
}
