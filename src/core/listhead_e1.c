/* Sort the gListHead02016280 list by key — 0x0800D530-0x0800D65F
 *
 * What it does: it empties the doubly linked list at 0x02016280 and rebuilds it
 * into the second list at 0x02016288 by insertion sort, finally writing the
 * sorted list's head back to 0x02016280.
 *
 * The sort key is 32 bits: the upper half is the score returned by
 * GetNodeBoxDistance (which is also written into the node's +0x1C field), and
 * the lower half is 255 minus the order value in the node's owner record
 * (+0x10 -> +0x14). The comparisons are UNSIGNED (`bls`/`bhi`), so the key is
 * u32. The list is in DESCENDING key order.
 *
 * The insertion point is not scanned from the front: starting from the
 * previously inserted node (prev), it walks forward (next) if the key is
 * larger and backward (prev) if it is smaller or equal. The two scans are
 * written symmetrically; those are the ROM's two separate loops.
 *
 * Node layout: +0x10 owner record, +0x14 next, +0x18 prev, +0x1C score.
 * +0x14/+0x18 match the Node in src/world/list_ops.c; the owner record's +0x14
 * field is the same block GetNodeBoxDistance reads.
 *
 * ADDRESS NOTE: 0x02016280 is recorded in data/ram_map.csv and was used as an
 * extern symbol (rule 1). 0x02016288 is NOT RECORDED and writing under data/
 * was not allowed in this session, so an address cast via #define was written,
 * as in src/core/list_b1.c. The folding trap does not arise here: no offset is
 * used from the address, and the ROM also loads it as a SEPARATE literal every
 * time. The pool layout (0x02016280, 0x02016288, 0x02016288, 0x02016280,
 * 0x02016288) came out identical to the ROM's.
 *
 * THE ONE MEASURED DETAIL -- why the `sorted` local exists:
 * The first version produced 296/304 and the ONLY difference was register
 * numbers: for us the constant 255 was in r7 and `next` in r8; in the ROM 255
 * is in r8 and `next` in r9. All eight bytes are the price of that shift
 * (prologue +2, `movs r0,#255`/`mov r8,r0` +2, `mov r2,r8` +2, epilogue +2) --
 * the instruction sequence was already the same.
 * The reason: the ROM takes the ADDRESS 0x02016288 into a pseudo at the start
 * of the forward scan and keeps it live throughout the loop (r7), whereas we
 * took it into a short-lived scratch (r1) right before the comparison. One
 * extra live value shifts the callee-saved list by one (the register table in
 * docs/COMPILER.md). Taking the address into an EXPLICIT local with
 * `sorted = gSortedHeadPtr;` -- and doing the comparison and the write-back
 * through `*sorted` -- produced that lifetime: 296 -> 304, exactly.
 * The position matters: the assignment must come AFTER `q = prev->next;` and
 * BEFORE the loop; in the ROM the `ldr r7,=...` sits exactly between those two
 * instructions.
 * The head insertion in the backward scan and the write-back at the end of the
 * function DO NOT USE `sorted` (they use the macro); the ROM reloads the
 * address from the pool there, and carrying `sorted` into those places would
 * lengthen its lifetime and break the match.
 *
 * AN ELIMINATED PATH: making 0x02016288 an extern symbol (the natural choice
 * under rule 1) could not be tried -- writing to data/ram_map.csv was not
 * allowed in this session. Nor was it needed: the #define address cast,
 * together with the `sorted` local above, gives exactly the code the ROM has.
 *
 * MATCH: 304/304 bytes, on the second attempt.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/listhead_e1.c
 */

#include "gba_types.h"

/* The complement of the lower half: a small order value means a large key. */
#define RANK_BIAS 255

/* The record the node points at from +0x10; only +0x14 is used. */
typedef struct Owner {
    u8  pad00[20];
    u32 rank;                   /* +0x14 */
} Owner;

typedef struct Node {
    u8  pad00[16];
    Owner *owner;               /* +0x10 */
    struct Node *next;          /* +0x14 */
    struct Node *prev;          /* +0x18 */
    u16 score;                  /* +0x1C */
} Node;

/* The head of the sorted list; not recorded in data/ram_map.csv (see ADDRESS NOTE). */
#define gSortedHeadPtr       ((Node **)0x02016288)
#define gSortedHead02016288  (*gSortedHeadPtr)

extern Node *gListHead02016280;

/* 0x0800D450: produces a score from the node's distance to the camera/region record. */
extern int GetNodeBoxDistance(Node *node);

#define SORT_KEY(n) \
    (((u32)(n)->score << 16) | (u32)(RANK_BIAS - (n)->owner->rank))

/* 0x0800D530 */
void SortListByKey(void)
{
    Node *cur;
    Node *next;
    Node *prev;
    Node *p;
    Node *q;
    Node *head;
    Node **sorted;
    u32 key;
    int score;

    if (gListHead02016280 == 0)
        return;

    /* The first node is set up as the sole element of the sorted list. */
    cur = gListHead02016280->next;
    gSortedHead02016288 = gListHead02016280;
    gListHead02016280->next = 0;
    gListHead02016280->prev = 0;
    gSortedHead02016288->score = GetNodeBoxDistance(gListHead02016280);
    prev = gListHead02016280;

    while (cur != 0) {
        next = cur->next;
        score = GetNodeBoxDistance(cur);
        cur->score = score;
        key = ((u32)score << 16) | (u32)(RANK_BIAS - cur->owner->rank);

        p = prev;
        if (SORT_KEY(prev) > key) {
            /* Forward scan: after the last node with a larger key. */
            q = prev->next;
            sorted = gSortedHeadPtr;
            while (q != 0 && SORT_KEY(q) > key) {
                p = q;
                q = q->next;
            }
            if (q == *sorted) {
                cur->next = q;
                cur->prev = 0;
                q->prev = cur;
                *sorted = cur;
            } else {
                cur->next = q;
                cur->prev = p;
                p->next = cur;
                if (q != 0)
                    q->prev = cur;
            }
        } else {
            /* Backward scan: in front of those with a smaller or equal key. */
            q = prev->prev;
            while (q != 0 && SORT_KEY(q) <= key) {
                p = q;
                q = q->prev;
            }
            if (q == 0) {
                head = gSortedHead02016288;
                cur->next = head;
                cur->prev = 0;
                head->prev = cur;
                gSortedHead02016288 = cur;
            } else {
                cur->next = p;
                cur->prev = q;
                p->prev = cur;
                q->next = cur;
            }
        }

        prev = cur;
        cur = next;
    }

    gListHead02016280 = gSortedHead02016288;
}
