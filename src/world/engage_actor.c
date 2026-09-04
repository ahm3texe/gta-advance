/* Aktoru devreye alma — 0x08055B34-0x08055B7D
 *
 * Iki kapi: +0x0B baytinin 12 maskesi 8 olmali, ve GetEntityKind sonucunun
 * 0. biti kurulu olmali. Ikisi de saglanirsa hedefin bayraklarinda bit 9
 * kurulup bit 0 siliniyor, SetActorEngage cagriliyor, sonra bit 7 de
 * kuruluyor ve 1 donuluyor.
 *
 * Kural 33: sabiti AYRI sonuc yereline koyup yerinde `&=` kullanmak
 * gerekiyor. ROM `movs r0,#12` ile maskeyi ONCE kuruyor, sonra alani
 * okuyup maskeyi kendi register'inda guncelliyor; `alan & 12` yazmak
 * sonucu alanin register'inda tutar ve farkli kod uretir.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * BLOK SIRASI ONEMLI: ROM'un basarisizlik blogu IKI kontrolun ARASINA
 * dusuyor -- ilk kontrol basarisizlikta ileri atliyor, ikinci kontrol
 * BASARIDA atliyor ve basarisizlik yoluna DUSEREK giriliyor. Iki ayri
 * `return 0` yazmak farkli dal mesafeleri uretiyordu (bne +0x32 yerine
 * ROM'da +0x0C). Etiketli bicim ROM'un kontrol akisini dogrudan ifade
 * ediyor; ayni cozum src/world/bump_or_reset.c'de de kullanilmisti.
 *
 * ROM cagridan SONRA +0x28'i YENIDEN okuyor; cagri onu degistirmis
 * olabilecegi icin bu dogal davranis.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/engage_actor.c
 */

#include "gba_types.h"

#define STATE_MASK  12
#define STATE_READY 8
#define KIND_BIT    1
#define FLAG_SET    (0x80 << 2)
#define FLAG_DONE   0x80

typedef struct Target {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
    u8  pad10[20];
    u32 unk24;                  /* +0x24 */
} Target;

typedef struct Actor {
    u8      pad00[11];
    u8      state;              /* +0x0B */
    u8      pad0C[24];
    u32     unk24;              /* +0x24 */
    Target *target;             /* +0x28 */
} Actor;

extern u32  GetEntityKind(u32 arg);
extern void SetActorEngage(u32 arg);

/* 0x08055B34 */
u32 EngageActor(Actor *actor)
{
    u32 state;
    Target *target;

    state = STATE_MASK;
    state &= actor->state;
    if (state != STATE_READY)
        goto fail;
    if (GetEntityKind(actor->unk24) & KIND_BIT)
        goto engage;

fail:
    return 0;

engage:
    target = actor->target;
    target->flags = (target->flags | FLAG_SET) & ~1;
    SetActorEngage(target->unk24);

    target = actor->target;
    target->flags |= FLAG_DONE;
    return 1;
}
