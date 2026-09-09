/* Notify other actors — 0x08019710-0x0801979B
 *
 * Walk 0x0202F2C0 then 0x0202F310. Call FUN_08037d98 for other actors whose
 * +0x2C partner is not busy and whose 0x400000 flag is clear. In the first
 * list, entity +0x30 must have zero at +0x08; in the second, record +0x18
 * must have mode 2 at +0x30 (same terminology as MarkDistantActors).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/ping_other_actors.c
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
