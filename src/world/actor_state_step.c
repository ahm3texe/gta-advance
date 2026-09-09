/* Advances the actor by one frame according to its state value.  0x08017628,
 * 1536 bytes.
 *
 * The skeleton: if the STATE at +0x28 is not 0x7FFF (empty), values above
 * 0x3FFF are passed through a remapping table, a large switch then runs on the
 * state, and the state is set back to 0x7FFF.  A shared tail follows.
 *
 * gcc turns the switch into a BINARY SEARCH TREE (`cmp #0x38 / bhi`, then
 * `cmp #0x18 / bhi` ...), which is why the ROM shows 26 distinct values and a
 * 35-link comparison chain -- in the source they are all plain `case`s.
 *
 * RULE 45 WAS APPLIED (docs/COMPILER.md): the helper pattern below appears in
 * NINE separate cases, and in the ROM each of them loads 0x020303C4 from ITS
 * OWN pool word -- that is, nine separate physical blocks.  Given a shared
 * local, agbcc merges them by cross-jumping and the blocks disappear; so every
 * case has ITS OWN locals.
 *
 * Note: Ghidra mistook the `bx r0` at the end of the function for an
 * "unrecoverable jump table".  It is not; the `pop {r4,r5,r6}; pop {r0}; bx r0`
 * sequence is simply the interworking form of a void return.
 *
 * Ghidra could not even define a function at this address: the automatic
 * analysis tried to decode it in ARM mode and gave up with "bad instruction
 * data".  The TMode register was set to 1 by hand and the region was
 * re-disassembled (tools/ghidra/ExportDecompileBatch.java).
 *
 * STATUS: 1494/1536, 42 bytes short.  The prologue, the dispatch tree and the
 * pool count have settled; the remaining difference is in the case BODIES.
 *
 *   prologue        push {r4,r5,r6,lr}  IDENTICAL
 *   tree root       0x38                IDENTICAL
 *   comparisons     91/92, 78 in the right order
 *   `mov pc,rX`     0 on both sides (no jump table)
 *   0x020303C4      pool 9/9
 *
 * THE THREE CHANGES THAT SETTLED THE TREE, in order:
 *
 *   1. `case STATE_IDLE` was added -> THE ROOT BECAME 0x38.  agbcc's
 *      balance_case_nodes picks the pivot as `(case + range + 1) / 2`; with 40
 *      cases the pivot came out as index 19 = 0x36.  Because the remapping
 *      table can produce STATE_IDLE, the ROM has an EMPTY-bodied case for it
 *      (the `uVar2 != uVar6` branch in Ghidra).  With 41 cases the pivot is
 *      index 20 = 0x38, which is the ROM's root.
 *
 *   2. Three groups were written in GNU RANGE form (0x21...0x29, 0x2f...0x30,
 *      0x4e...0x51).  A range node produces two SINGLE comparisons in the ROM,
 *      HIGH then LOW (41 then 33); written as separate cases the value appears
 *      TWICE (33, 33).  Ordered matching went 58 -> 72.
 *
 *   3. The cases were ordered according to the ROM's BODY LAYOUT: gcc emits the
 *      bodies in source order, and in the ROM the first body is the shared
 *      "9,1" block (0x08017824) with the default last (0x08017b40).  72 -> 78.
 *
 * What settled the prologue: the ROM keeps the constant 0x20 in r6 and pushes
 * {r4,r5,r6,lr}.  The value is written in four places, and agbcc lifts it into
 * a callee-saved register.  Taking it into a local had the same effect (the
 * common prefix went 0 -> 5 bytes).
 *
 * THE REMAINING 42 BYTES.  Writing the shared "9,1" body as a single copy (the
 * ROM has a single pool word for it too) removed 28 bytes; the ROM carries
 * 28 more bytes in that region, so the shape of the eight jump stubs is not
 * quite right yet.  There is also one comparison missing (91/92) and two bodies
 * have swapped places.
 *
 * THE DECISIVE FINDING -- 0x4005/0x4026/0x4027/0x4028 ARE ALSO CASES.
 * Ghidra shows them as pool constants (_DAT_080177d0 and its neighbours), but
 * they are nodes of the comparison tree.  Written without them, the case set
 * stays DENSE between 1 and 0x97 and agbcc produces a JUMP TABLE: 1814 bytes
 * (296 TOO MANY), 45 comparisons, one `mov pc,r0`.  With the four added, the
 * range opens to 1..0x4028 and gcc is forced to produce a tree: 1510 bytes, 92
 * comparisons, zero `mov pc`.  A single change worth 304 bytes.
 *
 * Case 0x0E DOES NOT APPEAR as `cmp #14` in the ROM; because it is the only
 * value left in the tree's (0x0D, 0x0F) range, it is separated with `< 0x0F`.
 * Do not remove it thinking it is a missing case.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_state_step.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define STATE_IDLE   0x7FFF
#define REMAP_FIRST  0x3FFF
#define REMAP_BASE   0x4000
#define REMAP_TABLE  0x08CA45CC
#define TABLE_18     0x083429F6
#define TABLE_17     0x083429D8
#define TASK_FLAG    0x400
#define CLEAR_8000   0xFFFF7FFF

typedef struct Visual {
    u8 pad0[0x26];
    u8 mode;                  /* +0x26 */
} Visual;

typedef struct Owner {
    u8 pad0[0x30];
    s8 phase;                 /* +0x30 */
} Owner;

typedef struct Link {
    u8 pad0[0x54];
    s32 ready;                /* +0x54 */
} Link;

typedef struct Task {
    u8 pad0[4];
    Link *link;               /* +0x04 */
    u8 pad8[0x20];
    u32 flags;                /* +0x28 */
} Task;

typedef struct Holder {
    u8 pad0[0x28];
    Task *task;               /* +0x28 */
} Holder;

typedef struct Entity {
    u8 pad0[0x0c];
    u32 flags;                /* +0x0C */
    u8 pad10[4];
    u8 **detail;              /* +0x14 */
    u8 pad18[4];
    u8 **kind;                /* +0x1C */
} Entity;

typedef struct Actor {
    Owner *owner;             /* +0x00 */
    u8 pad4[2];
    s16 prev;                 /* +0x06 */
    u8 tag;                   /* +0x08 */
    u8 pad9;
    s8 sub;                   /* +0x0A */
    u8 pad0b[5];
    s32 speed;                /* +0x10 */
    u8 pad14[0x14];
    u32 state;                /* +0x28 */
    u8 pad2c[4];
    Entity *entity;           /* +0x30 */
    u8 pad34[8];
    Visual *visual;           /* +0x3C */
} Actor;

extern s32 GetOwnerSlot(Entity *entity);
extern u8 *SelectSlotCD(void);
extern void RequestActorAction(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_080198e4(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_080426ec(Task *task, s32 a, Entity *entity);
extern s32 GetAnchorUnk34(void);
extern s32 FUN_08023660(Entity *entity);
extern void FUN_08016990(Actor *self, s32 a);
extern void DispatchActorBehavior(Actor *self);
extern void FUN_08019260(Actor *self);

/* RULE 45: every case must write this pattern with ITS OWN locals.  Because
 * the macro does not generate fresh names on each expansion, the locals are
 * declared at the call site and passed to the macro by name. */
/* A macro parameter CANNOT be `task`: the preprocessor would substitute it
 * inside the `->task` member name too and `->t` would come out.  Parameter
 * names must not collide with member names. */
#define NUDGE(NENT, NTSK)                                                   \
    do {                                                                    \
        (NTSK) = ((Holder *)*(u8 **)gRam020303C4)->task;                    \
        if ((NTSK) != 0) {                                                  \
            (NTSK)->flags |= TASK_FLAG;                                     \
            if ((NTSK)->link->ready != 0) FUN_080426ec((NTSK), 10, (NENT)); \
        }                                                                   \
    } while (0)

void FUN_08017628(Actor *self)
{
    u8 *slot;   /* SelectSlotCD's result; carries the table index at +0x1C */
    u32 state;
    s32 nine;
    /* The ROM keeps the constant 0x20 in r6 (callee-saved) and pushes
     * {r4,r5,r6,lr} in the prologue: the value is written in FOUR places
     * across the function, so agbcc lifts it into a register.  Taking it
     * into a local has the same effect. */
    u8 mode20;

    mode20 = 0x20;
    if (GetOwnerSlot(self->entity) == 0) return;
    slot = SelectSlotCD();
    if (self->visual != 0) self->visual->mode = mode20;

    state = self->state;
    if (state != STATE_IDLE) {
        if (state > REMAP_FIRST) {
            self->state = *(u16 *)(REMAP_TABLE + (state - REMAP_BASE) * 2);
        }
        state = self->state;
        switch (state) {
        /* The body order follows the ROM's layout: gcc emits case bodies in
         * SOURCE order.  In the ROM the first body is the shared "9,1" block
         * (0x08017824) and the default is last (0x08017b40). */

        /* THE SHARED BODY: eight cases share it and carry the value in a
         * register.  The ROM has a SINGLE physical copy (the pool word
         * _DAT_08017858), so a single copy is written here too -- rule 45
         * only applies when the ROM has SEPARATE copies. */
        case 0x02: nine = 0x02; goto body_nine;
        case 0x03: nine = 0x03; goto body_nine;
        case 0x2c: nine = 0x2c; goto body_nine;
        case 0x2d: nine = 0x2d; goto body_nine;
        case 0x5c: nine = 0x5c; goto body_nine;
        case 0x5d: nine = 0x5d; goto body_nine;
        case 0x67: nine = 0x67; goto body_nine;
        case 0x68: nine = 0x68; goto body_nine;
        body_nine: { Entity *e; Task *t;
            RequestActorAction(self, nine, 9, 1);
            e = self->entity; NUDGE(e, t); break; }

        /* EACH of the eight cases below loads 0x020303C4 from its own pool
         * word in the ROM -- separate physical blocks, so under rule 45 each
         * is given its own locals. */
        case 0x01: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x87, 1, 0x0f); break; }
        case 0x2b: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x8a, 1, 0x0f); break; }
        case 0x4b: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x91, 1, 0x0f); break; }
        case 0x4e ... 0x51:
                   { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x94, 1, 0x0f); break; }
        case 0x57: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x97, 1, 0x07); break; }
        case 0x5b: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x9a, 1, 0x0f); break; }
        case 0x66: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x8d, 1, 0x0f); break; }
        case 0x2f ... 0x30:
                   { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x90, 1, 0x47); break; }

        case 0x63: RequestActorAction(self, 0x63, 10, 2); break;
        case 0x4c: RequestActorAction(self, 0x4c, 10, 2); break;

        case 0x18: {
            u32 v18 = *(u16 *)(TABLE_18 + **(u8 **)(slot + 0x1c) * 2);
            if (v18 == 0) {
                RequestActorAction(self, 0x18, 2, 1);
            } else {
                RequestActorAction(self, 0x6c, 2, 1);
                FUN_080198e4(self, v18 + 8, 1, 0);
            }
            break;
        }

        case 0x21 ... 0x29:
        case 0x58: FUN_080198e4(self, 0x79, 1, 0x23); break;
        case 0x35: FUN_080198e4(self, 0x81, 1, 0x23); break;
        case 0x36: FUN_080198e4(self, 0x84, 1, 0x23); break;

        case 0x38:
            if (self->prev != 0x38 && self->prev != 0x96) self->sub = 2;
            RequestActorAction(self, 0x38, 3, 0);
            if (self->visual != 0) self->visual->mode = 0x40;
            break;

        case 0x0d: {
            Entity *e0d;
            RequestActorAction(self, 0x0d, 2, 1);
            if (self->visual != 0) self->visual->mode = 0x40;
            e0d = self->entity;
            if (e0d != 0 && (e0d->flags & 0x8000) != 0) e0d->flags &= CLEAR_8000;
            break;
        }

        case 0x4005:
        case 0x17: {
            u32 v17 = *(u16 *)(TABLE_17 + **(u8 **)(slot + 0x1c) * 2);
            if (v17 == 0) {
                RequestActorAction(self, state, 1, 1);
            } else {
                RequestActorAction(self, 0x6a, 1, 1);
                FUN_080198e4(self, v17 + 8, 1, 0x13);
            }
            if (self->visual != 0) self->visual->mode = mode20;
            break;
        }

        case 0x4027:
        case 0x96:
            if (self->prev != 0x96) self->sub = 2;
            RequestActorAction(self, 0x96, 3, 0);
            if (self->visual != 0) self->visual->mode = 0x40;
            break;

        case 0x4028:
        case 0x97: RequestActorAction(self, state, 1, 0); goto lit40;
        case 0x0e: RequestActorAction(self, 0x0e, 2, 1); goto lit40;
        case 0x0f: RequestActorAction(self, 0x0f, 2, 1); goto lit40;
        case 0x10: RequestActorAction(self, 0x10, 3, 1); goto lit40;
        case 0x0c: RequestActorAction(self, 0x0c, 3, 1); goto lit40;
        case 0x1f: RequestActorAction(self, 0x1f, 2, 0);
        lit40:
            if (self->visual != 0) self->visual->mode = 0x40;
            break;

        case 0x4026:
            RequestActorAction(self, state, 3, 1);
            if (self->visual != 0) self->visual->mode = mode20;
            break;

        case STATE_IDLE:
            break;

        default:
            RequestActorAction(self, state, 2, 2);
            break;
        }
        self->state = STATE_IDLE;
    }

    if (GetAnchorUnk34() == 0) {
        if ((u8)(self->owner->phase - 2) < 2) {
            if (self->visual != 0) self->visual->mode = 0x40;
            if (self->prev != 0x38) RequestActorAction(self, 0x20, 3, 0);
        } else if (FUN_08023660(self->entity) == 0
                   || (*(s8 *)(self->entity->detail + 0x114) != 2
                       && (u8)(self->sub - 2) < 2)) {
            if (FUN_08023660(self->entity) == 0) {
                DispatchActorBehavior(self);
                FUN_08019260(self);
            } else {
                self->tag = 3;
                RequestActorAction(self, 0x4000, 2, 3);
                if (self->visual != 0) self->visual->mode = mode20;
            }
        } else {
            if (GetOwnerSlot(self->entity) != 0
                && *(s8 *)(self->entity->detail + 0x114) == 2
                && self->speed > 0x30000) {
                self->speed = 0x30000;
            }
            if (self->visual != 0) self->visual->mode = 0x10;
        }
    }
    FUN_08016990(self, 0x10000);
}
