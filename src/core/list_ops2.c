/* Doubly linked list (with custom offsets) — 0x080127FC-0x0801282B
 *
 * A variant of the pattern in src/core/linked_list.c: the header is +0 head,
 * +4 tail, +8 count. Each node has +0 next and +4 prev. `ListInit` clears
 * the header; `PushFront` pushes the new node onto the FRONT of the list
 * and increments the count.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/list_ops2.c
 */

#include "gba_types.h"

typedef struct Node2 {
    struct Node2 *next;         /* +0x00 */
    struct Node2 *prev;         /* +0x04 */
} Node2;

typedef struct List2 {
    Node2 *head;                /* +0x00 */
    Node2 *tail;                /* +0x04 */
    int    count;               /* +0x08 */
} List2;

/* 0x080127FC */
void List2Init(List2 *list)
{
    list->tail  = 0;
    list->head  = 0;
    list->count = 0;
}

/* 0x08012808 — insert at the HEAD of the list. */
void List2PushFront(List2 *list, Node2 *node)
{
    Node2 *old;

    old = list->head;
    if (old == 0) {
        list->head = node;
        list->tail = node;
        node->next = 0;
        node->prev = 0;
    } else {
        node->next = old;
        node->prev = 0;
        old->prev = node;
        list->head = node;
    }

    list->count++;
}
