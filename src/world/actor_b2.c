/* Aktore yeni bir kimlik yukluyor ve iki ozel kimlikte bagli yapiyi isaretliyor.
 * 0x0801686C, 292 bayt.
 *
 * TASLAK -- olcum surerken guncellenecek.
 */

#include "gba_types.h"

typedef struct Entity {
    u8 pad0[4];
} Entity;

typedef struct Holder {
    u8 pad0[0x30];
    u8 kind;                  /* +0x30 */
} Holder;

typedef struct Owner {
    u8 pad0[0xa8];
    u8 mode;                  /* +0xA8 */
} Owner;

typedef struct Link {
    u8 pad0[0x0c];
    u32 flags;                /* +0x0C */
    u8 pad10[8];
    Holder *holder;           /* +0x18 */
    Owner *owner;             /* +0x1C */
} Link;

typedef struct Actor {
    u8 pad0[4];
    u16 current;              /* +0x04 */
    u16 request;              /* +0x06 */
    u8 rank;                  /* +0x08 */
    u8 slot;                  /* +0x09 */
    u8 state;                 /* +0x0A */
    u8 pad0b[5];
    u32 timer;                /* +0x10 */
    u8 pad14[0x1c];
    Entity *entity;           /* +0x30 */
    u8 pad34[4];
    u16 *table;               /* +0x38 */
    u8 pad3c[0x70];
    Link *link;               /* +0xAC */
} Actor;

extern void FUN_0803c400(Entity *entity);

void FUN_0801686c(Actor *self, u32 id, u32 slot, u32 rank)
{
    u32 value;
    u32 flags;
    Link *lnk;
    Link *volatile *slotp;

    if (id == 0x7fff) return;
    if (self->rank < rank && self->state != 2) return;
    if (rank == 0 && self->rank == 0 && self->state != 2) return;

    {
        u32 t;
        if (self->table != 0) {
            if (id > 0x3fff)
                t = self->table[id - 0x4000];
            else
                t = id;
        } else {
            t = id;
        }
        value = t;
    }

    if (value == 0x7fff) return;
    if (self->current == value && self->state != 2) {
        FUN_0803c400(self->entity);
        return;
    }

    self->timer = 0;
    self->current = value;
    self->request = id;
    self->slot = slot;
    self->rank = rank;
    self->state = 0;

    switch (id) {
    case 12:
    case 13:
    case 15:
        lnk = self->link;
        slotp = &self->link;
        if (lnk == 0) return;
        flags = lnk->flags;
        if ((flags & 0x80) == 0) {
            if ((flags & 1) == 0) return;
            if ((flags & 0x800) != 0) return;
            if (lnk->holder->kind == 2) return;
        }
        {
            Owner *own = (*slotp)->owner;
            own->mode = (own->mode & 0x3f) | 0x80;
        }
        break;
    case 16:
        lnk = self->link;
        slotp = &self->link;
        if (lnk == 0) return;
        flags = lnk->flags;
        if ((flags & 0x80) == 0) {
            if ((flags & 1) == 0) return;
            if ((flags & 0x800) != 0) return;
            if (lnk->holder->kind == 2) return;
        }
        {
            Owner *own = (*slotp)->owner;
            own->mode = (own->mode & 0x3f) | 0x40;
        }
        break;
    }
}
