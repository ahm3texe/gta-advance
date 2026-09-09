/* Player slot queries — 0x0806543C-0x08065499
 *
 * Both queries use GetOwnerSlot to determine the entity's player, obtain that
 * player's slot from SelectSlotAB, then inspect flag +0x25. The second first
 * tests type byte +0x138 in the entity's own +0x14 block and may return 1 early.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/player_slot_query.c
 */

#include "gba_types.h"

typedef struct PlayerSlot {
    u8 pad00[0x25];
    u8 marked;                  /* +0x25 */
} PlayerSlot;

typedef struct EntityBlock {
    u8 pad00[0x138];
    u8 kind;                    /* +0x138 */
} EntityBlock;

typedef struct Entity {
    u8           pad00[0x14];
    EntityBlock *block;         /* +0x14 */
} Entity;

extern u32   GetOwnerSlot(void *entity);
extern void *SelectSlotAB(u32 which);

/* 0x0806543C */
s32 IsOwnerMarked(void *entity)
{
    PlayerSlot *slot;

    slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(entity));
    if (slot != 0 && slot->marked != 0)
        return 1;

    return 0;
}

/* 0x0806545C */
s32 IsEntityEngaged(Entity *entity)
{
    PlayerSlot *slot;
    s32         result;

    slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(entity));

    /* The ROM places the zero block first (rule 49), so the condition tests
 * not engaged; the return-1 branch must remain at the tail.
 */
    if ((entity == 0 || entity->block == 0 || entity->block->kind > 3) &&
        (slot == 0 || slot->marked == 0))
        result = 0;
    else
        result = 1;

    return result;
}
