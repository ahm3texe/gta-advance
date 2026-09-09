/* Flag the owner, then run three steps — 0x08038440-0x0803846B
 *
 * Bit 23 of the owner's +0x18 flags is set when there is an owner at +0x2C;
 * the three steps run either way. 0x800000 is `movs #128 / lsls #16`.
 *
 * The entity stays in r4 across all three calls, which is what the `push {r4}`
 * pays for.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entity/flag_owner_and_step.c
 */

#include "gba_types.h"

#define OWNER_FLAG  (128 << 16) /* 0x800000 */

typedef struct StepOwner {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} StepOwner;

typedef struct StepEntity {
    u8         pad00[0x2C];
    StepOwner *owner;           /* +0x2C */
} StepEntity;

extern void FUN_08036ba0(StepEntity *entity);
extern void FUN_080367cc(StepEntity *entity);
extern void FUN_0803685c(StepEntity *entity);

/* 0x08038440 */
void FUN_08038440(StepEntity *entity)
{
    StepOwner *owner = entity->owner;

    if (owner != 0)
        owner->flags |= OWNER_FLAG;
    FUN_08036ba0(entity);
    FUN_080367cc(entity);
    FUN_0803685c(entity);
}
