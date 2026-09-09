/* Unlink the node and put it on the free list — 0x08012968-0x080129FD  [MATCHED]
 *
 * Removes the node from the doubly linked list, then adds it to the head of
 * the free list. The removal happens with interrupts DISABLED (REG_IME 0 -> 1).
 *
 * There are four paths: removal from the middle (next and prev present),
 * removal from the head (next only), removal from the tail (prev only), and a
 * single element (neither). All of them join at a common re-enable point,
 * which is why the control flow is written WITH LABELS.
 *
 * Offsets: the list head at +0x804 is loaded FROM THE POOL, while the free
 * list at +0x800 is BUILT with `movs r0,#128 / lsls r0,#4`. Writing the two in
 * the same form produces different code.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * THE THREE MEASUREMENTS THAT OPENED THE MATCH (53 -> 25 -> 20 -> 0)
 * -----------------------------------------------------------------
 * (1) `base = &gNodePool;` IS NOT WRITTEN IN THE BRANCHES; it is written AT
 *     THE MERGE POINT.
 *     The `ldr r2,[pc,#x]` instructions at the end of the ROM's four branches
 *     are NOT FOUR SOURCE ASSIGNMENTS; they are agbcc PLACING a SINGLE
 *     merge-point constant assignment at the end of every predecessor. Written
 *     in the branches, `base`'s allocno "dies nowhere" because of its four
 *     definition sites (the greg dump has NO "dies in N places" note) and its
 *     live_length balloons to 256; its priority of 2*6/256 = 0.047 drops it to
 *     last and it cannot claim r2 in time. Moved to the merge point, the order
 *     corrects itself: base takes r2, prev2 moves to r3 and node to r4.
 *     MEASUREMENT: base in the branches -> live_length 256, priority 0.047,
 *     order 7/7.
 * (2) A SEPARATE prev LOCAL PER BRANCH (rule 45): the no_next path's prev is a
 *     separate local (`prev2`). The ROM uses r0 in the first branch and r3 in
 *     the no_next branch -- which means TWO SEPARATE allocnos.
 * (3) A SEPARATE local (`head`) for the free list head. The previous version
 *     reused `next`; that raised next's refs from 5 to 11 and its lifetime
 *     from 12 to 23, pushing it to r2 instead of r1.
 * (4) In the `neither` branch the store goes DIRECTLY THROUGH THE SYMBOL
 *     (`gNodePool.activeHead = prev2;`). That makes the address computation
 *     use a pseudo separate from base. On its own it already took 53 -> 25.
 *
 * PATHS TRIED AND ELIMINATED (do not retry)
 * -----------------------------------------
 * - Removing the `base` local entirely and using the symbol directly
 *   everywhere: 114 bytes, MUCH WORSE. The local is needed -- but AT THE MERGE
 *   POINT.
 * - `base = &gNodePool;` in all four branches + `base->activeHead = ...`: 53
 *   differences.
 * - The same plus the direct symbol in `neither`: 25 differences.
 * - Writing it with structured if/else: four different forms were tried (fully
 *   structured, outer goto with inner structured, outer structured with inner
 *   goto, mixed) -- ALL gave THE SAME 20 differences. agbcc normalises the
 *   control-flow form; the block order does NOT CHANGE from here.
 * - `slot = &gNodePool.activeHead; *slot = prev2;` in the `neither` branch:
 *   142 bytes, the size breaks.
 * - Putting the base assignment BEFORE the store in the `neither` branch: 54
 *   differences.
 * - Writing next instead of prev2 in the `neither` branch: the difference does
 *   not change (20).
 * - Removing the `slot` local in the tail and writing `base->freeHead`: 20
 *   differences.
 * - Swapping the order of `node->prev`/`node->next` in the tail: 25
 *   differences, worse.
 * - The declaration order of the source locals: it shifts the pseudo numbers
 *   but, because there is no priority tie, DOES NOT CHANGE the allocation.
 * - The first attempt gave 148 bytes: inverting the free-list condition (the
 *   ROM enters the zero case by FALLING THROUGH and skips the non-zero one
 *   with `bne`) fixed the size. That form must be preserved.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/unlink_to_free.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

/* 0x08012968 */
void UnlinkToFree(Node *node)
{
    Node *next;
    Node *prev;
    Node *prev2;
    NodePool *base;
    Node *head;
    Node **slot;

    if (node == 0)
        return;

    REG_IME = 0;

    next = node->next;
    if (next == 0)
        goto no_next;

    prev = node->prev;
    if (prev == 0)
        goto no_prev;

    next->prev = prev;
    node->prev->next = node->next;
    goto reenable;

no_prev:
    next->prev = prev;
    gNodePool.activeHead = node->next;
    goto reenable;

    /* RULE 45: this branch's prev is a SEPARATE local. The ROM uses r0 in
       the first branch and r3 here; a single local ties them into one allocno
       and leaves node in r3 instead of r4. */
no_next:
    prev2 = node->prev;
    if (prev2 == 0)
        goto neither;
    prev2->next = next;
    goto reenable;

neither:
    gNodePool.activeHead = prev2;

    /* base is assigned HERE, NOT in the branches. See header note (1). */
reenable:
    base = &gNodePool;
    REG_IME = 1;

    /* ORDER: the ROM enters the zero case by FALLING THROUGH (skipping the
       non-zero one with `bne`). Writing `if (head != 0)` emits `beq` and the
       block order is reversed.
       `head` is a SEPARATE local: reusing `next` pushed it to r2. */
    slot = &base->freeHead;
    head = *slot;
    if (head == 0) {
        *slot = node;
        node->next = head;
        node->prev = head;
        return;
    }
    node->next = head;
    node->prev = 0;
    *slot = node;
}
