/* Varlik durum ayarlayicilari — 0x0803F6F8-0x0803F78B
 *
 * Dort kucuk yaprak: her biri varligin +8'deki isleyici isaretcisini ve
 * +0x80..0x82'deki durum baytlarini kuruyor.
 *
 * Isleyici adresleri Thumb biti kurulu (tek sayi) olarak saklaniyor, bu
 * yuzden `extern` fonksiyon adi yerine tam degerli #define kullanildi:
 * agbcc_build .equ ile cift adres uretir ve son bit tutmaz.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_states.c
 */

#include "gba_types.h"

typedef struct Actor Actor;
typedef void (*ActorHandler)(Actor *actor);

/* Isleyiciler; ucu de Ghidra'nin kacirdigi fonksiyonlar. */
#define HANDLER_IDLE     ((ActorHandler)0x0803DF35)   /* 0x0803DF34 */
#define HANDLER_RELEASE  ((ActorHandler)0x0803DEA9)   /* 0x0803DEA8 */
#define HANDLER_ENGAGE   ((ActorHandler)0x0803E281)   /* 0x0803E280 */
#define HANDLER_ACTIVE   ((ActorHandler)0x0803F0E9)   /* 0x0803F0E8 */

#define OWNER_FLAG_BUSY  0x00800000

#define STATE_IDLE       0
#define STATE_ENGAGE     2
#define STATE_ACTIVE     3

typedef struct Owner {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Owner;

struct Actor {
    Owner       *owner;         /* +0x00 */
    u8           pad04[4];
    ActorHandler handler;       /* +0x08 */
    u8           pad0C[0x26];
    u16          unk32;         /* +0x32 */
    u8           pad34[8];
    u16          unk3C;         /* +0x3C */
    u16          unk3E;         /* +0x3E */
    u8           pad40[0x40];
    u8           phase;         /* +0x80 */
    u8           state;         /* +0x81 */
    u8           timer;         /* +0x82 */
};

/* 0x0803F6F8 */
void SetActorEngage(Actor *actor)
{
    if (actor == 0)
        return;

    actor->handler = HANDLER_ENGAGE;
    actor->state = STATE_ENGAGE;
    actor->phase = STATE_ENGAGE;
}

/* 0x0803F714 */
void SetActorActive(Actor *actor)
{
    if (actor == 0)
        return;
    if (actor->handler == HANDLER_ACTIVE && actor->state == STATE_ACTIVE)
        return;

    actor->handler = HANDLER_ACTIVE;
    actor->state = STATE_ACTIVE;
    actor->phase = 0;
    actor->unk3E = 0;
    actor->unk3C = 0;
    actor->timer = 0;
}

/* 0x0803F74C */
void SetActorIdle(Actor *actor)
{
    if (actor == 0)
        return;

    actor->handler = HANDLER_IDLE;
    actor->state = STATE_IDLE;
}

/* 0x0803F764 */
void ReleaseActor(Actor *actor)
{
    if (actor == 0)
        return;

    actor->owner->flags &= ~OWNER_FLAG_BUSY;
    actor->handler = HANDLER_RELEASE;
    actor->state = STATE_IDLE;
    actor->unk32 = 0;
}
