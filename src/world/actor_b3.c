/* Selecting the entity's animation node and dispatching the rest of the setup
 * -- 0x08015598-0x080157B7
 *
 * Takes the Actor structure set up by InitActor (src/world/actor_init.c),
 * selects a node from the definition tree in the ROM (rooted at 0x08BD3448)
 * and attaches it to the entity, then dispatches the rest of the setup to six
 * separate functions.
 *
 * The flow as read from the ROM:
 *   1. gRam03000078 -> the local "idx" (0 or 1).  If a->slots[idx] is 0xFDFD
 *      (the "unassigned" value written by InitActor) it moves to the other
 *      slot.
 *   2. Root -> group -> entry -> node: a three-level pointer table.  The same
 *      three-level access is in src/world/entries_a8.c; the `desc` there and
 *      the `node` here are objects at the same level (both go to FUN_08013cfc
 *      and to FUN_08014ee4 with the +0x14 field).
 *   3. If the parent context blocks the setup, a fixed ROM node is used
 *      instead of the node.
 *   4. The a->attr flags and the FUN_08014ee4 selection, then the six
 *      sub-calls.
 *
 * BYTE-MATCHING.
 *
 * THE MEASURED POINTS:
 *   - The `ldrsh` at 0x12: a->pos is 16.16 fixed point and its integer part is
 *     read as a separate s16 field.  Writing `(s16)(raw >> 16)` produces
 *     `asrs` while the ROM has `ldrsh` -- a union is required, a shift does
 *     not do it.
 *   - The pointer validity tests take the form `(u32)p - base <= length`:
 *     agbcc builds that as `movs #0xFE; lsls #24; adds; cmp; bls`.  The range
 *     order is from the ROM: EWRAM,IWRAM for `a`; ROM,EWRAM,IWRAM for
 *     node->unk10.  Changing the order leaves a difference.
 *   - THE BRANCH ORDER: in two places the condition had to be written
 *     INVERTED.  The ROM's inner (fall-through) branch is the
 *     `IsEntityEngaged(...) == 0` and `a->unk98 == 0` side; writing the right
 *     side first went 542 -> 538 -> 544.  The `!= 0` spelling produces the
 *     same instructions but lays the blocks out in reverse.
 *   - THE `self` COPY IS REQUIRED (the ROM: `mov r9, r4` at entry, `mov r0, r9`
 *     at the fifth call).  Copy-based splitting is usually eliminated; IT IS
 *     NOT ELIMINATED HERE, because the copy is used nowhere until the 5th call
 *     and the rest of the function has already filled r4-r7, so the second
 *     allocno moves to r8/r9.  Removing `self` gives 544 -> 538 (4 bytes of
 *     prologue/epilogue + alignment).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_b3.c
 */

#include "gba_types.h"

/* The memory regions: this is how a pointer is tested for actually pointing
 * at a readable area (the three consecutive range tests in the ROM). */
#define IN_ROM(p)    ((u32)(p) - 0x08000000 <= 0x00FFFFFF)
#define IN_EWRAM(p)  ((u32)(p) - 0x02000000 <= 0x0003FFFF)
#define IN_IWRAM(p)  ((u32)(p) - 0x03000000 <= 0x00007FFF)
#define IS_RAM_PTR(p)    (IN_EWRAM(p) || IN_IWRAM(p))
#define IS_LOADED_PTR(p) (IN_ROM(p) || IN_EWRAM(p) || IN_IWRAM(p))

/* the a->unk30->unk0C flags */
#define CTX_BLOCKED   0x00000400
#define CTX_ENABLED   0x00000001
#define CTX_OVERRIDE  0x00008000

/* The default node; used when the parent context blocks the setup. */
#define FALLBACK_NODE ((RomNode *)0x08CA45A8)

#define SLOT_UNSET    0xFDFD    /* the "unassigned" value written by InitActor */
#define MODE_SPECIAL  15
#define MODE_BIT      4
#define ATTR_BIT      8
#define NODE_KIND_MAX 2

/* 16.16 fixed point; the integer part is also read as an s16. */
typedef union Fixed16 {
    s32 raw;                    /* +0x00 */
    struct {
        u16 frac;               /* +0x00 */
        s16 whole;              /* +0x02 */
    } part;
} Fixed16;

/* --- The ROM definition tree: +0x00 a count and +0x04 a pointer table at
   each level --- */

typedef struct RomNode {
    u8   pad00[0x0C];
    u8   kind;                  /* +0x0C  must be 1 or 2 */
    void *unk10;                /* +0x10  must point at a loaded area */
    u32  unk14;                 /* +0x14 */
} RomNode;

typedef struct RomEntry {
    u8        count;            /* +0x00  the frame count */
    u8        pad01[3];
    RomNode **nodes;            /* +0x04 */
} RomEntry;

typedef struct RomGroup {
    u32        count;           /* +0x00 */
    RomEntry **entries;         /* +0x04 */
} RomGroup;

typedef struct RomRoot {
    u32        pad00;
    RomGroup **groups;          /* +0x04 */
} RomRoot;

extern RomRoot gRom08BD3448;

/* --- RAM --- */

extern u32 gRam03000078;        /* the index of the slot to pick (0/1) */
extern u32 gRam02000224;        /* the global mode flags */

/* --- The context and view structures --- */

typedef struct Context {
    u8  pad00[0x0C];
    u32 flags;                  /* +0x0C */
} Context;

typedef struct AttrOwner {
    u8  pad00[3];
    u8  unk03;                  /* +0x03 */
} AttrOwner;

typedef struct Attr {
    u8         pad00[0x20];
    u16        bits;            /* +0x20 */
    u8         pad22[0x16];
    AttrOwner *owner;           /* +0x38 */
} Attr;

typedef struct Actor {
    u32      unk00;             /* +0x00 */
    u16      unk04;             /* +0x04  the entry index */
    u16      unk06;             /* +0x06 */
    u8       unk08;             /* +0x08 */
    u8       mode;              /* +0x09 */
    u8       pad0A[2];
    s32      unk0C;             /* +0x0C  the frame count << 16 */
    Fixed16  pos;               /* +0x10 */
    u32      unk14;             /* +0x14 */
    u8       pad18[4];
    u32      unk1C;             /* +0x1C */
    u16      slots[2];          /* +0x20  0xFDFD = unassigned */
    RomGroup *group;            /* +0x24 */
    u32      unk28;             /* +0x28 */
    u8       pad2C[4];
    Context *ctx;               /* +0x30 */
    u32      unk34;             /* +0x34 */
    u32      unk38;             /* +0x38 */
    Attr    *attr;              /* +0x3C */
    u8       pad40[0x50];
    u32      unk90;             /* +0x90 */
    u8       pad94[4];
    u32      unk98;             /* +0x98 */
    u8       pad9C[0x14];
    u8       attrSlot;          /* +0xB0 */
    u8       padB1[3];
    RomNode *node;              /* +0xB4 */
} Actor;

extern u32   FUN_08013cfc(Attr *attr, RomNode *node, u32 idx);
extern void  FUN_08014ee4(Attr *attr, u32 value);
extern Attr *FUN_08028f98(u8 slot);
extern u32   IsEntityEngaged(Context *ctx);
extern void  FUN_08015a84(Actor *a, RomNode *node, u32 idx);
extern void  FUN_08015af8(Actor *a, RomNode *node, u32 idx);
extern void  UpdateActorLane(Actor *a, RomNode *node, u32 idx);
extern void  NoOp080197FC(Actor *a, RomNode *node, u32 idx);
extern void  UpdateActorSlotEntry(Actor *a, RomNode *node, u32 idx);
extern void  TryLaunchActor(Actor *a, u32 idx);

/* 0x08015598 */
void UpdateActorFrame(Actor *a)
{
    RomGroup *group;
    RomEntry *entry;
    RomNode *node;
    Attr *attr;
    AttrOwner *owner;
    s32 limit;
    u32 idx;
    u32 kind;
    Actor *self;

    self = a;               /* see the header: the 5th call goes through r9
                               in the ROM */
    idx = gRam03000078;

    if (a->ctx->flags & CTX_BLOCKED)
        return;
    if (a == 0)
        return;
    if (!IS_RAM_PTR(a))
        return;
    if (a->ctx == 0)
        return;
    if ((a->ctx->flags & CTX_ENABLED) == 0)
        return;

    if (a->slots[idx] == SLOT_UNSET)
        idx = (idx == 0);

    group = gRom08BD3448.groups[a->slots[idx]];
    a->group = group;
    if (a->unk04 >= group->count)
        return;

    entry = group->entries[a->unk04];
    limit = entry->count << 16;
    a->unk0C = limit;
    if (a->pos.raw >= limit)
        a->pos.raw = limit - 1;

    node = entry->nodes[a->pos.part.whole];
    if (node == 0)
        return;

    kind = node->kind;
    if (kind == 0)
        return;
    if (kind > NODE_KIND_MAX)
        return;

    if (!IS_LOADED_PTR(node->unk10))
        return;

    if ((a->ctx != 0 && (a->ctx->flags & CTX_OVERRIDE)) ||
        (a->mode == MODE_SPECIAL && (gRam02000224 & MODE_BIT) == 0))
        node = FALLBACK_NODE;

    a->node = node;
    if (FUN_08013cfc(a->attr, node, idx) == 0)
        return;

    if (a->attr != 0) {
        if (IsEntityEngaged(a->ctx) == 0) {
            a->attr->bits &= ~ATTR_BIT;
            attr = FUN_08028f98(a->attrSlot);
            if (attr != 0)
                attr->bits &= ~ATTR_BIT;
        } else {
            a->attr->bits |= ATTR_BIT;
            attr = FUN_08028f98(a->attrSlot);
            if (attr != 0)
                attr->bits |= ATTR_BIT;
        }
    }

    if (a->unk98 == 0) {
        if (a->unk90 == 0) {
            FUN_08014ee4(a->attr, node->unk14);
        } else {
            FUN_08014ee4(a->attr, a->unk90);
        }
    } else {
        FUN_08014ee4(a->attr, a->unk98);
        owner = a->attr->owner;
        if (owner != 0)
            owner->unk03 = 1;
    }

    FUN_08015a84(a, node, idx);
    FUN_08015af8(a, node, idx);
    UpdateActorLane(a, node, idx);
    NoOp080197FC(a, node, idx);
    UpdateActorSlotEntry(self, node, idx);
    TryLaunchActor(a, idx);
}
