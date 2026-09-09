/* Loads a new id into the actor; for ids 12/13/15 and 16 it marks the mode
 * byte in the owner of the linked structure.
 * 0x0801686C, 292 bytes.  BYTE-MATCHING.
 *
 * THE FLOW
 *   0x7FFF is the sentinel value; if the id is above 0x3FFF, its entry is read
 *   from the translation table at the actor's +0x38 (id - 0x4000) to find the
 *   real id.  If the id is already running it merely touches the entity and
 *   returns.  Otherwise the counter is cleared, id/request/slot/rank are
 *   written, the state is cleared, and for two id families the owner of the
 *   linked structure is marked.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_b2.c
 *
 * THREE MEASURED PATTERNS (each verified individually, do not change them):
 *
 * 1) THE `t` TEMPORARY IS REQUIRED (rule 69).  At the merge point the ROM
 *    keeps two copies: `adds r0,r1,#0` on the else branch and `adds r2,r0,#0`
 *    at the merge.  Dropping `t` and writing straight into `value` makes both
 *    branches produce into value's register, and two instructions disappear
 *    at once (280/292).
 *
 * 2) THE TABLE TEST MUST BE TWO NESTED `if`s, NOT `&&`.  Two separate `t = id`
 *    spellings are needed: after reordering, the two fuse into a single block
 *    by cross-jumping, i.e. THE GENERATED CODE IS THE SAME; the only thing
 *    that changes is `id`'s reference count, 11 -> 12.  That lifts the rule 50
 *    priority from 0.493 to 0.522, past `value`'s 0.500, and the allocation
 *    becomes the ROM's (id r1, value r2, slot r5).  With `&&` the order is
 *    reversed: id r5, value r1 -- half the body drifts.
 *    Measurement: python3 tools/dump_alloc.py src/world/actor_b2.c RequestActorAction
 *
 * 3) THE LINKED STRUCTURE IS RE-READ IN THE TAIL.  The ROM keeps the `+0xAC`
 *    ADDRESS in r3 and RELOADS the pointer at the merge point
 *    (`ldr r0,[r3,#0]`), whereas agbcc's CSE keeps the loaded value in hand
 *    even where the two branches merge.  Two pieces are needed:
 *      - the read must go through `volatile` so CSE cannot eliminate it;
 *      - the address must be kept in a SEPARATE local (`slotp`) and that local
 *        must be at FUNCTION SCOPE.  With a single definition at block scope,
 *        local-alloc eliminates the copy, reduces the address to one register
 *        and `adds r3,r0,#0` disappears (288/292).  With two definitions in
 *        two cases the copy survives.
 *
 * PATHS RULED OUT (do not try them again):
 *   - producing `value` directly in the if/else     -> 280, 62/144
 *   - a separate local per branch (`mapped`/`plain`) -> value has 5 references,
 *                                                       priority 0.556, reversed allocation
 *   - dropping the `flags` local for `self->link->flags` -> no difference
 *   - `Link **held = &self->link;` (without volatile) -> CSE eliminates it again
 *   - a function-scope local named `owner` in the tail -> an extra address
 *                                                       copy appears, 114/144
 *   - a function-scope `mask` local                 -> the mask lands in r2,
 *                                                       the ROM uses r0
 *   - two separate volatile reads (load+store)      -> 302 bytes
 */

#include "gba_types.h"

/* The entity at the actor's +0x30; only its address is passed here, so its
 * internal layout has not been worked out (sibling: src/world/actor_reset.c). */
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

extern void GetOwnerSlot(Entity *entity);

void RequestActorAction(Actor *self, u32 id, u32 slot, u32 rank)
{
    u32 value;                /* the translated id */
    u32 flags;                /* the linked structure's +0x0C flags */
    Link *lnk;                /* the linked structure, for the gate tests */
    /* The address for the re-read in the tail.  Function scope and volatile:
     * see pattern (3) in the file header for why. */
    Link *volatile *slotp;

    if (id == 0x7fff) return;                 /* the sentinel id */
    /* The rank comparison is UNSIGNED (bcs); rule 31. */
    if (self->rank < rank && self->state != 2) return;
    if (rank == 0 && self->rank == 0 && self->state != 2) return;

    /* The id translation.  Two nested `if`s and two separate `t = id`
     * spellings are mandatory; see patterns (1) and (2) in the header. */
    {
        u32 t;
        if (self->table != 0) {
            if (id > 0x3fff)
                t = self->table[id - 0x4000];   /* the entry is offset by 0x4000 */
            else
                t = id;
        } else {
            t = id;
        }
        value = t;
    }

    if (value == 0x7fff) return;              /* the translation gave the sentinel too */
    /* The id is already running: the entity is notified and we return. */
    if (self->current == value && self->state != 2) {
        GetOwnerSlot(self->entity);
        return;
    }

    /* The write order is the ROM's: counter, id, request, slot, rank, state. */
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
        /* The address is captured first, the pointer re-read afterwards. */
        lnk = self->link;
        slotp = &self->link;
        if (lnk == 0) return;
        flags = lnk->flags;
        /* Bit 0x80 opens the gate directly; otherwise all three must hold. */
        if ((flags & 0x80) == 0) {
            if ((flags & 1) == 0) return;
            if ((flags & 0x800) != 0) return;
            if (lnk->holder->kind == 2) return;
        }
        {
            /* `own` is at block scope: a function-scope local here would
             * produce an extra address copy (see the rejected paths in the
             * header). */
            Owner *own = (*slotp)->owner;
            own->mode = (own->mode & 0x3f) | 0x80;   /* the low 6 bits are preserved */
        }
        break;
    case 16:
        /* The address is captured first, the pointer re-read afterwards. */
        lnk = self->link;
        slotp = &self->link;
        if (lnk == 0) return;
        flags = lnk->flags;
        /* Bit 0x80 opens the gate directly; otherwise all three must hold. */
        if ((flags & 0x80) == 0) {
            if ((flags & 1) == 0) return;
            if ((flags & 0x800) != 0) return;
            if (lnk->holder->kind == 2) return;
        }
        {
            /* `own` is at block scope: a function-scope local here would
             * produce an extra address copy (see the rejected paths in the
             * header). */
            Owner *own = (*slotp)->owner;
            own->mode = (own->mode & 0x3f) | 0x40;   /* the low 6 bits are preserved */
        }
        break;
    }
}
