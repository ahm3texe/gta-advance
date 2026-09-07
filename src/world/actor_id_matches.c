/* Kimlik esleme sorgusu — 0x08019620-0x0801966F
 *
 * Tutucunun +0x1C'deki varligini iki alanindan (u32 +0x28 ve u16 +0x06)
 * verilen kimlige karsi sinar. Kimlik 0x3FFF'ten buyukse once
 * gRom08CA45CC[id - 0x4000] ile bir takma ada cevrilip o da denenir.
 *
 * VARLIK IKI KEZ OKUNUYOR: ROM ikinci turda +0x1C'yi yeniden yukluyor
 * (ldr r1,[r4,#28]), yani kaynak ikinci turda AYRI bir yerel kullaniyor.
 * Tek yerelde tutulursa ayni yazmac (r2) yeniden kullaniliyor.
 *
 * Ayrica erken "return 0" cikisi ic ice `if (e != 0) { ... }` olarak
 * yazilmali; ayri bir erken donusle blok sirasi tersine doniyor
 * (return 1 ve return 0 bloklari yer degistiriyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_id_matches.c
 */

#include "gba_types.h"

#define ID_DIRECT_MAX  0x3FFF
#define ID_BASE        0x4000

typedef struct Ent {
    u8  pad00[6];
    u16 tag;                    /* +0x06 */
    u8  pad08[0x20];
    u32 id;                     /* +0x28 */
} Ent;

typedef struct Holder {
    u8   pad00[0x1C];
    Ent *ent;                   /* +0x1C */
} Holder;

extern const u16 gRom08CA45CC[];

/* 0x08019620 */
u32 ActorIdMatches(Holder *h, u32 id)
{
    Ent *e;

    e = h->ent;
    if (e != 0) {
        if (id > ID_DIRECT_MAX) {
            u16 t;

            t = gRom08CA45CC[id - ID_BASE];
            if (e->id == t)
                return 1;
            if (e->tag == t)
                return 1;
        }
        {
            Ent *cur;

            cur = h->ent;
            if (cur->id == id)
                return 1;
            if (cur->tag == id)
                return 1;
        }
    }
    return 0;
}
