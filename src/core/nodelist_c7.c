/* Acquire a node from the free list for an id and walk to the end of the chain
 * 0x08054608-0x080546CB  (196 bytes)
 *
 * The sibling: GetOrCreateRecordNode (src/core/nodelist_b5.c, verified
 * byte-matching).
 * The shared body (id scan + taking over the spare node + the FUN_080521c4
 * call) was taken from there; this function has two extra pieces:
 *   1. At entry, the SPARE_ID (0x7FEF) id itself is rejected.
 *   2. Before returning, the node's +0x30 chain is walked to its end.
 *
 * PATHS TRIED / ELIMINATED  (whoever tries something new should NOT REPEAT
 * these and should ADD their own attempts to this list rather than deleting
 * it):
 *   - None; the first writing matched exactly with the form below (196/196).
 *     The choices below were made by READING THE ROM, not by guessing:
 *     * `flag = 2; flag &= node->kind;` -- rule 33. The ROM emits
 *       `movs r0,#2` / `ldrb r1` / `ands r0,r1`: the result is in THE
 *       CONSTANT's register.
 *     * `owner->flags & 1`, on the other hand, was written PLAINLY. The ROM
 *       produces the inverse here: `ldr r0,[r0,#12]` / `movs r1,#1` /
 *       `ands r0,r1` -- the result is in THE VALUE's register. So rule 33 is
 *       NOT APPLIED to this second mask; within the same function the two masks
 *       want two different forms.
 *     * The tail walk was written `node = node->link;`, NOT `node = cur;`. The
 *       ROM RE-READS the field with `ldr r4,[r4,#48]`; writing `node = cur`
 *       would produce an `adds r4,r0,#0` copy instead.
 *     * The loop form was NOT COPIED from the sibling: the scan loop takes a
 *       `goto test` that skips the entry, and the tail walk takes a separate
 *       `goto walk_test` -- the ROM has two separate `b` instructions
 *       (0x8054620, 0x80546b4).
 */

#include "gba_types.h"

#define SPARE_ID    0x7FEF
#define ENTRY_SIZE  64

/* Only the +0x0C flag word of the object at +0x28 is used; no names were
 * invented for the rest, which was left as padding. */
typedef struct Owner {
    u8  pad00[0x0C];                /* +0x00 */
    u32 flags;                      /* +0x0C bit0: skip the chain walk */
} Owner;

typedef struct Node {
    struct Node  *next;             /* +0x00 */
    struct Node  *prev;             /* +0x04 */
    u16           key;              /* +0x08 ID */
    u8            slot;             /* +0x0A */
    u8            kind;             /* +0x0B flag byte */
    u8            pad0C[0x1C];      /* +0x0C..0x27 */
    Owner        *owner;            /* +0x28 */
    u8            pad2C[4];         /* +0x2C */
    struct Node  *link;             /* +0x30 child/continuation chain */
} Node;

typedef struct Entry {
    u8 pad00[ENTRY_SIZE];
} Entry;

typedef struct RecordTable {
    u8     pad00[4];                /* +0x00 */
    int    count;                   /* +0x04 upper bound of valid ids */
    u8     pad08[0x14];             /* +0x08..0x1B */
    Entry *entries;                 /* +0x1C */
} RecordTable;

#define RECORD_TABLE  ((const RecordTable *)0x08D49C00)

extern Node *gList02035A80;         /* 0x02035A80 list head */

extern void ListRemove(Node **list, Node *node);
extern void InsertSorted(Node **list, Node *node, s32 id);
extern void FUN_080521c4(Node *node, const Entry *entry);

/* 0x08054608 */
Node *ResolveRecordNodeChain(s32 id)
{
    Node **list;
    Node  *cur;
    Node  *spare;
    Node  *node;
    Owner *owner;
    s32    k;
    s32    flag;

    /* The spare node's own id is not a valid search key. */
    if (id == SPARE_ID)
        goto none;

    if (id >= RECORD_TABLE->count)
        goto none;

    list = &gList02035A80;
    cur = list[0];
    spare = list[1];
    goto test;

step:
    cur = cur->next;
test:
    if (cur == 0)
        goto scanned;
    if (cur->key == id)
        goto found;
    if (cur->key <= id)
        goto step;

scanned:
    /* The list is exhausted: if the spare node is still free, it is taken over. */
    if (spare->key == SPARE_ID)
        goto insert;
    goto none;

found:
    node = cur;
    goto check;

insert:
    ListRemove(list, spare);
    spare->key = id;
    k = spare->kind & 15;
    spare->slot = 0;
    k = (u8)(k | 2);
    k = k & ~1;
    spare->kind = k;
    InsertSorted(list, spare, id);
    node = spare;

check:
    if (node != 0)
        goto body;

none:
    return 0;

body:
    flag = 2;
    flag &= node->kind;
    if (flag != 0)
        FUN_080521c4(node, RECORD_TABLE->entries + id);

    /* If the owner object has bit0 set, the node is returned as it is. */
    owner = node->owner;
    if (owner != 0) {
        if ((owner->flags & 1) != 0)
            goto done;
    }

    /* Otherwise it walks to the last link of the +0x30 chain.
     * A self-referencing link (cur == node) never starts the walk. */
    cur = node->link;
    if (cur == node)
        goto done;
    goto walk_test;

walk_step:
    node = node->link;
    cur = node->link;
walk_test:
    if (cur != 0)
        goto walk_step;

done:
    return node;
}
