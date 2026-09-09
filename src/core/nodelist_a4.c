/* Put the node into "ready" mode and release it. 0x08052750, 216 bytes. MATCHED.
 *
 * Same family as RebuildAreaEntry (src/core/nodelist_b6.c, 0x080526B8); it sits
 * 152 bytes away. What they share: the `index >= gAreaBank.count` gate, the
 * gList02035A80 (0x02035A80) list head and the node's +0x0B "kind" byte.
 * The difference: this function SEARCHES for the node (FindNodeAfter,
 * 0x08055A94), passes it through a series of gates and finally notifies with
 * FUN_08055D90.
 *
 * DETAILS MEASURED FROM THE ROM
 *   - gAreaBank is at 0x08D49C00 in the ROM; +0x04 is an s32 counter (the same
 *     view as in nodelist_c3/d5/d6). `ldr r0,=0x08D49C00` + `ldr r0,[r0,#4]`,
 *     i.e. a plain member access.
 *   - The list head is gList02035A80; both the search and the notification are
 *     passed its ADDRESS (`ldr r0,=0x02035A80`), not its value. The same
 *     pattern as b6.
 *     (NOT gNodeListHead at 0x02035A70 -- a neighbour, but a separate object.)
 *   - The return type is VOID: the exit is `pop {r4-r7}; pop {r0}; bx r0` and
 *     no path writes a value into r0 (rule 35).
 *   - The 0x100 mask is built as `movs #128 / lsls #1` and the 0x400 mask as
 *     `movs #128 / lsls #3`; neither is a pool constant.
 *   - In the else branch `str r3` writes zero twice: r3 = bits & 0x100, and the
 *     branch is only entered while r3 == 0. A plain `= 0` is enough in the
 *     source; agbcc's cse recognises the "register is zero" equality coming
 *     from the condition and reuses r3 without emitting an extra `movs`.
 *
 * THE FIRST WRITING GAVE 208 BYTES (45 of 103 instructions differing). Two
 * separate levers were found; both account for the 8 bytes:
 *
 * 1) THE +0x0B FIELD MUST BE SIGNED (s8) -- the INVERSE direction of rule 47.
 *    With a `u8 kind`, `kind &= ~1` folds into a single instruction:
 *    `movs r0,#254`.
 *    The ROM instead builds -2 with `movs r0,#2 / negs r0,r0`, i.e. the mask is
 *    at INT width. Making it s8 produces the ROM's two instructions (+2 bytes).
 *    There is NO cost in the other direction: `kind & 1`, `kind & 0xF1` and
 *    `(kind & 0xF) | 0x10` still emit `ldrb`, and agbcc adds no sign extension
 *    for a mask below 0x100. (b6 saw the same field as s8 and noted that the
 *    signedness made no difference there -- HERE IT DOES.)
 *
 * 2) A SEPARATE LOCAL POINTER FOR THE LOOP BRANCH (it fits the rule 45
 *    pattern).
 *    Without `Node *n = node;`, agbcc keeps the node in r5 and the index in r6,
 *    never uses r7, and CSEs the `node->shape` load out of the condition block
 *    into the body -- a SINGLE `ldr [.,#44]` per iteration.
 *    The ROM has two (one in the body for the list, one in the latch for the
 *    counter) and a third in the pre-header. Adding the branch local produces
 *    the ROM's exact shape:
 *      adds r5,r6,#0 / movs r4,#0 / ldr r0,[r6,#44] / b .Lcond
 *    and the index moves to r7, making it `push/pop {r4-r7}` (+6 bytes).
 *    So the `adds r5,r6,#0` REGISTER COPY was one that CAN be produced from
 *    the source: the price is a separate local inside the branch.
 *
 * THE LOOP FORM: agbcc's classic rotated for -- shape is loaded in the
 * pre-header and control branches to the condition (`b .Lcond`), with the body
 * at 0x080527C4 and the condition at 0x080527D6. The sibling b6 has no loop at
 * all; the form COULD NOT have been copied from it.
 *
 * FORMS I TRIED AND ELIMINATED
 *   - `u8 kind` (see 1): 2 bytes short, `movs #254`.
 *   - Plain `node->...` without the branch local (see 2): 6 bytes short, the
 *     loop body one `ldr` shorter, r7 unused, `push {r4,r5,r6,lr}`.
 *   - I did not try taking a local BEFORE the loop as
 *     `Shape *shape = node->shape;`: since the ROM re-reads it every iteration,
 *     that form is eliminated from the start (the memory invalidation between
 *     calls is plainly visible in the ROM).
 *
 * STILL OPEN -- A SYMBOL DECLARATION IS NEEDED
 *   0x020110C0 is NOT in data/ram_map.csv (the nearest is 0x020110AC =
 *   gRam020110AC, a separate literal). Only its ADDRESS is passed as an
 *   argument here, with no member access, so a raw cast does not disturb the
 *   pool constant and does not prevent the match. I did not touch ram_map; a
 *   symbol name needs to be assigned.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_a4.c  -> BYTE-MATCHING
 */
#include "gba_types.h"

#define BITS_BUSY    0x02        /* +0x18 bit 1: busy -> do nothing */
#define KIND_ARMED   0x01        /* +0x0B bit 0 */
#define KIND_LOW     0x0F
#define KIND_READY   0x10
#define KIND_TEST    0xF1
#define KIND_MATCH   0x11
#define BITS_LIST    0x100       /* +0x18 bit 8: the shape list path */
#define SUB_DONE     0x400       /* flag at +0x0C in the sub-object */

/* The shape descriptor at +0x2C; only the fields that are used were named. */
typedef struct Shape {
    u8   pad00[13];
    u8   count;                 /* 0x0D */
    u8   pad0E[2];
    u16 *list;                  /* 0x10 */
} Shape;

/* Sub-object at +0x28. */
typedef struct Sub {
    u8  pad00[9];
    u8  unk09;                  /* 0x09 */
    u8  pad0A[2];
    u32 flags;                  /* 0x0C */
    u8  pad10[0x1C];
    u32 unk2C;                  /* 0x2C */
} Sub;

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8   pad04[7];
    s8   kind;                  /* 0x0B */
    u8   pad0C[12];
    u32  bits;                  /* 0x18 */
    u16  unk1C;                 /* 0x1C */
    u8   pad1E[2];
    u32  unk20;                 /* 0x20 */
    u8   pad24[4];
    Sub *sub;                   /* 0x28 */
    Shape *shape;               /* 0x2C */
} Node;

typedef struct AreaBank {
    u8  pad00[4];
    s32 count;                  /* 0x04 */
} AreaBank;

extern AreaBank gAreaBank;              /* 0x08D49C00 (ROM table) */
extern Node    *gList02035A80;          /* 0x02035A80 */

/* 0x020110C0 has NO symbol in data/ram_map.csv (the nearest is 0x020110AC =
 * gRam020110AC). Only its ADDRESS is passed as an argument, with no member
 * access, so a raw cast does not disturb the pool constant. A symbol name
 * needs to be assigned; I am not touching ram_map, this is reported. */
#define gRam020110C0 ((u32 *)0x020110C0)

extern Node *FindNodeAfter(Node *node, s32 id);   /* 0x08055A94 */
extern u32   GetOwnerSlot(u32 sub);               /* 0x0803C400 */
extern void  ForwardZeroArg2(u32 id);             /* 0x08055BF8 */
extern void  FUN_0800c804(u32 *dest, u32 value);  /* 0x0800C804 */
extern void  FUN_08055d90(u32 *head, u32 id);     /* 0x08055D90 */

/* 0x08052750 */
void ReleaseAreaNode(s32 index, u32 arm)
{
    Node *node;
    s32 i;

    if (index >= gAreaBank.count) return;

    node = FindNodeAfter((Node *)&gList02035A80, index);
    if (node == 0) return;
    if ((node->bits & BITS_BUSY) != 0) return;
    if (node->sub != 0 && GetOwnerSlot((u32)node->sub) != 0) return;

    if ((node->kind & KIND_ARMED) != 0 && arm != 0)
        node->kind = (node->kind & KIND_LOW) | KIND_READY;

    if ((node->kind & KIND_TEST) == KIND_MATCH) {
        if ((node->bits & BITS_LIST) != 0) {
            Node *n = node;
            for (i = 0; i < n->shape->count; i++)
                ForwardZeroArg2(n->shape->list[i]);
            n->kind &= ~KIND_ARMED;
        } else {
            Sub *sub = node->sub;
            if (sub != 0) {
                node->unk1C = sub->unk09;
                sub->unk2C = 0;
                sub->flags |= SUB_DONE;
                node->sub = 0;
            }
        }

        if (node->unk20 != 0) {
            FUN_0800c804(gRam020110C0, node->unk20);
            node->unk20 = 0;
        }
    }

    FUN_08055d90((u32 *)&gList02035A80, index);
}
