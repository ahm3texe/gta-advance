/* Are all four tracked halfwords zero — 0x08005F74-0x08005FA7
 *
 * Reads element 0 of each pair first, then element 1 of each, and answers 1
 * only if every one of them is zero. The placeholder name is kept: what the
 * two pairs measure is not known.
 *
 * The register-offset loads (`movs r1,#0 / ldrsh r0,[r2,r1]`) are not a choice
 * in the source. Thumb's ldrsh has no immediate-offset form at all, so even a
 * constant subscript has to go through a register.
 *
 * Rule 49: the single `return 0` sits at the end and is reached by forward
 * branches from all four tests.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/bootstrap/both_pairs_zero.c
 */

#include "gba_types.h"

extern s16 gUnk02000EA8[2];
extern s16 gUnk02000CD8[2];

/* 0x08005F74 */
u32 FUN_08005f74(void)
{
    if (gUnk02000EA8[0] != 0) goto no;
    if (gUnk02000CD8[0] != 0) goto no;
    if (gUnk02000EA8[1] != 0) goto no;
    if (gUnk02000CD8[1] != 0) goto no;
    return 1;
no:
    return 0;
}
