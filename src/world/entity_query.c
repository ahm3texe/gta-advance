/* Varlik sorgusu — 0x08055AF8-0x08055B33
 *
 * Varligin +0x0B turune gore dallaniyor: 8 ise alt nesnenin degeri,
 * degilse +0x24 -> +0x14 -> +0x22 zincirindeki bayrak, o da yoksa
 * karo sorgusu.
 *
 * BYTE-MATCHING. Maske sabitini ayri bir u32 yerele atayip yerinde `&=`
 * yapmak, sonucu ROM'daki gibi sabitin register'inda (`r0`) tutuyor.
 *
 * Ilk derlemede 36 bayt farkliydi; ozel dali fonksiyonun SONUNA almak
 * (kosulu ters cevirerek) 2'ye indirdi. Kalan fark, `flags & 3` ifadesi
 * yerine `mask = 3; mask &= flags` ile kapandi.
 *
 * Ayni kumedeki eslesen dort fonksiyon: src/world/node_search.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entity_query.c
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
