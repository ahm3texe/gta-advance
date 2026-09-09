/* Insert into an ordered list — 0x0801282C-0x08012895
 *
 * It inserts a node into a doubly linked list ordered by key and increments the
 * count. Three paths: an empty list, insertion in the middle, insertion at the
 * end.
 *
 * The control flow is written WITH LABELS, as in the ROM. The loop is ROTATED
 * in the ROM: tested once at entry and again at the end of the body. Writing a
 * structured `while` makes agbcc produce a different block order -- the same
 * situation was measured in src/world/engage_actor.c and
 * src/world/bump_or_reset.c.
 *
 * The count (`count`) is read before the call and comes from the same place on
 * all THREE paths; the ROM keeps it in r5.
 *
 * THE KEY PARAMETER MUST BE u32, NOT u16. Writing `u16 key` makes agbcc
 * truncate the parameter at entry (`lsls r2,#16` + `lsrs r2,#16`, exactly 4
 * bytes), and the ROM has none of that. `strh` already writes the low 16 bits,
 * and the comparison naturally stays unsigned through the `ldrh` result.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/insert_sorted.c
 */

#include "gba_types.h"

typedef struct Node {
    struct Node *next;          /* +0x00 */
    struct Node *prev;          /* +0x04 */
    u16          key;           /* +0x08 */
} Node;

typedef struct List {
    Node *head;                 /* +0x00 */
    Node *tail;                 /* +0x04 */
    u32   count;                /* +0x08 */
} List;

/* 0x0801282C */
void InsertSorted(List *list, Node *node, u32 key)
{
    Node *cur;
    Node *next;
    u32 count;

    node->key = key;

    if (list->head == 0) {
        list->head = node;
        list->tail = node;
        node->next = 0;
        node->prev = 0;
        count = list->count;
        goto done;
    }

    /* THE ORDER matters: the ROM reads cur->next FIRST and the count SECOND.
       Writing the count first swapped the positions of the two loads. */
    cur = list->head;
    next = cur->next;
    count = list->count;
    if (next == 0)
        goto check_tail;
    if (cur->key > key)
        goto insert_before;

loop:
    cur = cur->next;
    if (cur->next == 0)
        goto check_tail;
    if (cur->key <= key)
        goto loop;

check_tail:
    if (cur->key <= key)
        goto insert_after;

insert_before:
    node->next = cur;
    node->prev = cur->prev;
    if (cur->prev != 0)
        cur->prev->next = node;
    cur->prev = node;
    if (list->head == cur)
        list->head = node;
    goto done;

insert_after:
    cur->next = node;
    node->prev = cur;
    node->next = 0;
    list->tail = node;

done:
    list->count = count + 1;
}
