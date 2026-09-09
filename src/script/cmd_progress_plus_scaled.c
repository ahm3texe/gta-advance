/* Script command: add a scaled operand to a progress field
 * 0x08059F8C-0x08059FB7
 *
 * The operand is multiplied by gRam02035AC4 when that is non-zero and left
 * alone when it is zero, so the global acts as a scale with 0 standing for 1
 * rather than for 0.
 *
 * The sibling at 0x0805A024 SUBTRACTS an unscaled operand from the same +0x0C
 * word of the progress block.
 *
 * Rule 53: `muls r1,r0` is the product written with the operands the other way
 * round in the source.
 *
 * Rule 70, with the refinement C89 forces: the progress pointer has to be a
 * local, or the cast folds the +0x0C into the pool constant and the load loses
 * its displacement, but a local ASSIGNED at the top pulls the pool load ahead of
 * the multiply. Declaring it at the top of the block (which C89 requires) and
 * assigning it at its point of use gives both. The sibling at 0x0805A024 wants
 * the load early and assigns at the top.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_progress_plus_scaled.c
 */

#include "gba_types.h"

#define PROGRESS_VALUE  3       /* +0x0C, as a u32 index */

extern u8  gRam02025810[];
extern u32 gRam02035AC4;

extern void FUN_08030ae4(u32 value, u32 slot);

/* 0x08059F8C */
u32 FUN_08059f8c(u32 a, u16 amount)
{
    u32 *progress;
    u32 scale = gRam02035AC4;
    u32 value = amount;

    if (scale != 0)
        value = scale * value;
    progress = (u32 *)gRam02025810;
    FUN_08030ae4(progress[PROGRESS_VALUE] + value, 1);
    return 1;
}
