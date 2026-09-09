/* Entity query — 0x08055AF8-0x08055B33
 *
 * Branch on the type at entity +0x0B: for 8, return the sub-object's value;
 * otherwise inspect the flag along +0x24 -> +0x14 -> +0x22, falling back to
 * a tile query if absent.
 *
 * BYTE-MATCHING. Assigning the mask constant to a separate u32 local and
 * applying &= in place keeps the result in the constant's register (r0),
 * as in the ROM. The first build differed by 36 bytes. Moving the special
 * branch to the END by inverting the condition reduced that to 2. Replacing
 * `flags & 3` with `mask = 3; mask &= flags` closed the remaining difference.
 *
 * Four matching functions in the same cluster: src/world/node_search.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entity_query.c
 */

#include "gba_types.h"

#define KIND_MASK        0x0C
#define KIND_SPECIAL     8
#define SUB_FLAG_MASK    3
#define SUB_FLAG_SHIFT   8

typedef struct SubTarget {
    u8 pad00[0x22];
    u8 flags;                   /* +0x22 */
} SubTarget;

typedef struct Holder {
    u8         pad00[0x14];
    SubTarget *target;          /* +0x14 */
} Holder;

typedef struct Entity {
    u8      pad00[11];
    u8      kind;               /* +0x0B */
    u8      pad0C[0x18];
    Holder *holder;             /* +0x24 */
} Entity;

extern u32 FUN_08042470(u32 arg);
extern u32 GetEntityUnk0C(Holder *holder);

/* 0x08055AF8 */
u32 QueryEntity(Entity *entity, u32 fallback)
{
    Holder *holder;
    u32 mask;
    u8 flags;

    if ((entity->kind & KIND_MASK) != KIND_SPECIAL) {
        holder = entity->holder;
        if (holder != 0) {
            if (holder->target != 0) {
                flags = holder->target->flags;
                if (flags != 0) {
                    mask = SUB_FLAG_MASK;
                    mask &= flags;
                    return mask << SUB_FLAG_SHIFT;
                }
            }
        }

        return FUN_08042470(fallback);
    }

    return GetEntityUnk0C(entity->holder);
}
