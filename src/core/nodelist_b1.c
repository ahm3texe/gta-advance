/* Shut down the node and its attached sub-objects. 0x08052828, 352 bytes. MATCHED.
 *
 * Same family as the sibling ReleaseAreaNode (src/core/nodelist_a4.c,
 * 0x08052750): the same `index >= gAreaBank.count` gate, the same
 * gList02035A80 list head, the same FindNodeAfter (0x08055A94) search, the same
 * +0x18 flag word and the same 0x020110C0 tail call. The differences: there is
 * one MORE entry gate here (index == 0x7FEF), the function CALLS ITSELF (for
 * every sub-id in the shape list) and it also clears the "owner" node at +0x30.
 *
 * DETAILS MEASURED FROM THE ROM
 *   - The first gate is `ldr r0,=0x7fef / cmp r2,r0 / bne` -- an equality test
 *     with a pool constant. Rule 44 (take the comparison constant into a
 *     local) IS NOT NEEDED HERE: canonicalization only happens in `<`/`<=`
 *     tests; `==` is left alone.
 *   - The +0x18 field is SIGNED (s32): the second gate is `cmp r1,#0 / bge`,
 *     i.e. `bits < 0`. A masked form (`bits & 0x80000000`) does not produce
 *     that branch.
 *   - +0x30 IS AN OWNER NODE and carries the same Node layout: through r4 both
 *     `[r4,#40]` (+0x28 sub-object) and `[r4,#24]` (+0x18 flags) are read. So
 *     it is `Node *owner` -- not a separate type.
 *   - The `owner = node->owner;` ASSIGNMENT must come BEFORE the `bits < 0`
 *     gate: the ROM emits `ldr r4,[r5,#48]` before `ldr r1,[r5,#24]`
 *     (rule 11/16).
 *   - The loop form: a ROTATED for with `movs r4,#0 / b .Lcond`. There is NO
 *     shape load in the pre-header; the shape is read in the condition block
 *     and CSEd into the body (the body uses the condition's r0 directly with
 *     `ldr r0,[r0,#16]`).
 *     In the sibling a4 the ROM does a third load in the pre-header, which is
 *     why a branch-local (`Node *n = node;`) was needed THERE; it is NOT NEEDED
 *     HERE -- an example of rule 49's warning against copying a loop form from
 *     a sibling.
 *     The confirmation is in the prologue: `push {r4,r5,r6,lr}` here (no r7)
 *     versus `push {r4-r7,lr}` in a4.
 *   - 0x08041EE0 TAKES ONE ARGUMENT. There is an explicit `adds r0,r2,#0`
 *     before the second call; there is none before the first because the
 *     sub-object is already allocated to r0.
 *     src/core/nodelist_a8.c declared it `void NoOp08041EE0(void)` and matched
 *     (r0 happened to be loaded there) -- HERE an argument-less declaration
 *     gives an 8-byte difference (measured).
 *   - The masks are at int width: `movs #17 / negs` = ~0x10, `movs #129 / negs`
 *     = ~0x80. Because the fields are u32, rule 47 requires nothing.
 *   - Two negative constants in the pool: 0xFFFFFEFF = ~0x100 and
 *     0xFFFFFEEF = ~0x110. The second is NOT ~0x111 -- that single byte was
 *     what broke the match.
 *   - The two branches SHARE the `node->bits |= <constant>` tail (cross-jumping
 *     at 0x08052964). The inverse of rule 29: the ROM has already done the
 *     merge, so writing a plain if/else in the source is correct.
 *
 * FORMS I TRIED AND ELIMINATED (measured)
 *   - The three zero stores to `sub->anim` are emitted IN REVERSE OF SOURCE
 *     ORDER. Writing them 0x00, 0x04, 0x08 does not produce the ROM's 0x08,
 *     0x04, 0x00 order: 2 bytes of difference. They were written in reverse in
 *     the source. (The same-direction effect as rule 8.)
 *   - Inverting it as `else if (arm != 0) {...} else {...}`: the function drops
 *     to 348 bytes with 80 bytes differing. agbcc does not invert the
 *     condition; the ROM's `cmp r6,#0 / bne` only comes out when the source has
 *     the `arm == 0` branch FIRST.
 *   - `NoOp08041EE0()` without an argument: 8 bytes of difference (above).
 *   - The +0x0B field as `u8` versus `s8`: NO DIFFERENCE, both give 0. The
 *     field is only read with `& 2` and never written; rule 47's lever does not
 *     exist here. It was left `s8` to stay consistent with the `s8` choice in
 *     a4 -- in this file that is NOT EVIDENCE, only consistency.
 *
 * STILL OPEN -- A SYMBOL DECLARATION IS NEEDED
 *   0x020110C0 is NOT in data/ram_map.csv (a4 left the same note for the same
 *   gap). Only its ADDRESS is passed as an argument, with no member access, so
 *   a raw cast does not disturb the pool constant. I did not touch ram_map.
 *   The meaning of the 0x80/0x01/0x100/0x40/0x10/0x110/0x4000 bits in the
 *   sub-object is unknown; they were named neutrally SUB_A..SUB_G, and no
 *   meaning was INVENTED.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b1.c  -> BYTE-MATCHING
 */
#include "gba_types.h"

#define INDEX_NONE   0x7FEF      /* first gate: an invalid id */
#define KIND_BUSY    0x02        /* +0x0B bit 1 */
#define BITS_SIGN    0x80000000  /* +0x18 sign bit: tested with `cmp/bge` */
#define BITS_HOLD    0x40000000
#define BITS_LIST    0x100
#define BITS_SUB     0x400
#define SUB_A        0x80
#define SUB_B        0x01
#define SUB_C        0x100
#define SUB_D        0x40
#define SUB_E        0x10
#define SUB_F        0x110
#define SUB_G        0x4000

typedef struct Anim {
    u32 unk00;
    u32 unk04;
    u32 unk08;
} Anim;

typedef struct Sub {
    u8    pad00[12];
    u32   flags;                /* 0x0C */
    u8    pad10[8];
    Anim *anim;                 /* 0x18 */
} Sub;

typedef struct Shape {
    u8   pad00[13];
    u8   count;                 /* 0x0D */
    u8   pad0E[2];
    u16 *list;                  /* 0x10 */
} Shape;

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8   pad04[7];
    s8   kind;                  /* 0x0B */
    u8   pad0C[12];
    s32  bits;                  /* 0x18 */
    u8   pad1C[4];
    u32  unk20;                 /* 0x20 */
    u8   pad24[4];
    struct Sub  *sub;           /* 0x28 */
    struct Shape *shape;        /* 0x2C */
    struct Node *owner;         /* 0x30 */
} Node;

typedef struct AreaBank {
    u8  pad00[4];
    s32 count;                  /* 0x04 */
} AreaBank;

extern AreaBank gAreaBank;              /* 0x08D49C00 (ROM table) */
extern Node    *gList02035A80;          /* 0x02035A80 */

#define gRam020110C0 ((u32 *)0x020110C0)

extern Node *FindNodeAfter(Node *node, s32 id);      /* 0x08055A94 */
extern s32   FUN_08055888(Node *node, s32 mode);     /* 0x08055888 */
extern void  NoOp08041EE0(Sub *sub);                 /* 0x08041EE0 */
extern void  FUN_0800c804(u32 *dest, u32 value);     /* 0x0800C804 */

/* 0x08052828 */
void DeactivateAreaNode(s32 index, u32 arm)
{
    Node *node;
    Node *owner;
    Sub  *osub;
    Sub  *sub;
    s32   i;

    if (index == INDEX_NONE) return;
    if (index >= gAreaBank.count) return;

    node = FindNodeAfter((Node *)&gList02035A80, index);
    if (node == 0) return;
    if ((node->kind & KIND_BUSY) != 0) return;

    owner = node->owner;
    if (node->bits < 0) return;
    if ((node->bits & BITS_HOLD) != 0 && arm == 0) return;

    if (owner != 0 && owner->sub != 0 && (owner->bits & BITS_SUB) != 0
        && FUN_08055888(owner, 0) == 0) {
        osub = owner->sub;
        osub->flags &= ~SUB_E;
        NoOp08041EE0(osub);
    }

    if ((node->bits & BITS_LIST) != 0) {
        for (i = 0; i < node->shape->count; i++)
            DeactivateAreaNode(node->shape->list[i], 1);
    }

    sub = node->sub;
    if (sub != 0) {
        if ((node->bits & BITS_SUB) != 0) {
            if (sub->anim != 0) {
                sub->anim->unk08 = 0;
                sub->anim->unk04 = 0;
                sub->anim->unk00 = 0;
            }
            if ((sub->flags & SUB_A) != 0)
                sub->flags &= ~SUB_A;
            if ((sub->flags & SUB_B) != 0)
                sub->flags = (sub->flags & ~SUB_C) | SUB_D;
            sub->flags &= ~SUB_E;
            NoOp08041EE0(sub);
        } else if (arm == 0) {
            sub->flags = (sub->flags & ~SUB_F) | SUB_G;
            node->bits |= BITS_HOLD;
        } else {
            sub->flags |= BITS_SUB;
            node->bits |= BITS_SIGN;
        }
    }

    if (node->unk20 != 0) {
        FUN_0800c804(gRam020110C0, node->unk20);
        node->unk20 = 0;
    }
    node->owner = 0;
}
