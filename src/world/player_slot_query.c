/* Oyuncu yuvasi sorgulari — 0x0806543C-0x08065499
 *
 * Iki sorgu. Ikisi de once varligin hangi oyuncuya ait oldugunu
 * GetOwnerSlot ile bulup SelectSlotAB'dan o oyuncunun yuvasini aliyor,
 * sonra yuvadaki +0x25 bayrağina bakiyor.  Ikincisi ondan once varligin
 * kendi +0x14 blogundaki +0x138 tur baytini sinayip erken 1 donuyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/player_slot_query.c
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

    /* ROM sifir govdesini once koyuyor (kural 49), bu yuzden kosul
     * "engaged degil" bicimindedir; 1 dalinin kuyrukta kalmasi sarttir. */
    if ((entity == 0 || entity->block == 0 || entity->block->kind > 3) &&
        (slot == 0 || slot->marked == 0))
        result = 0;
    else
        result = 1;

    return result;
}
