/* Diger aktorlere bildirim — 0x08019710-0x0801979B
 *
 * Iki listeyi (0x0202F2C0, sonra 0x0202F310) geziyor; kendisi disindaki,
 * +0x2C ortagi mesgul olmayan ve 0x400000 kurulu olmayan aktorler icin
 * FUN_08037d98 cagiriyor. Ilk listede olcut +0x30 varliginin +0x08'i sifir,
 * ikincide +0x18 kaydinin +0x30 kipi 2 (MarkDistantActors ile ayni sozluk).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/ping_other_actors.c
 */

#include "gba_types.h"
#define FLAG_ABORT   0x400000
#define OTHER_BUSY   0x2000000
#define MODE_READY   2
typedef struct Detail { u8 pad00[0x30]; u8 mode; } Detail;
typedef struct Other { u8 pad00[24]; u32 flags; } Other;
typedef struct Entity { u8 pad00[8]; u32 unk08; } Entity;
typedef struct Actor {
    struct Actor *next; u8 pad04[8]; u32 flags; u8 pad10[8]; Detail *detail;
    u8 pad1c[16]; Other *other; Entity *entity;
} Actor;
extern Actor *GetUnk0202F2C0(void);
extern Actor *GetUnk0202F310(void);
extern void   FUN_08037d98(Actor *actor);
void PingOtherActors(Actor *self)
{
    Actor *actor; Other *other;
    actor = GetUnk0202F2C0();
    while (actor != 0) {
        other = actor->other;
        if ((other == 0 || !(other->flags & OTHER_BUSY))
         && actor != self
         && actor->entity->unk08 == 0
         && !(actor->flags & FLAG_ABORT))
            FUN_08037d98(actor);
        actor = actor->next;
    }
    actor = GetUnk0202F310();
    while (actor != 0) {
        other = actor->other;
        if ((other == 0 || !(other->flags & OTHER_BUSY))
         && actor != self
         && actor->detail->mode == MODE_READY
         && !(actor->flags & FLAG_ABORT))
            FUN_08037d98(actor);
        actor = actor->next;
    }
}
