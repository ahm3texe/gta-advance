/* Doubly linked list — 0x08012898-0x08012943
 *
 * A node: +0 next, +4 prev. The list header: +0 head, +4 tail, +8 count.
 * The traversal functions copy the node pointer first, so the callback is free
 * to unlink the node.
 *
 * Indirect calls go to the `bl _call_via_rN` veneer; that is agbcc's way of
 * calling a function pointer with -mthumb-interwork.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/linked_list.c
 */

#include "gba_types.h"

typedef struct ListNode {
    struct ListNode *next;      /* +0 */
    struct ListNode *prev;      /* +4 */
} ListNode;

typedef struct List {
    ListNode *head;             /* +0 */
    ListNode *tail;             /* +4 */
    int       count;            /* +8 */
} List;

/* 0x08012898 */
void ListAppend(List *list, ListNode *node)
{
    ListNode *tail;

    if (list->head == 0) {
        list->head = node;
        list->tail = node;
        node->next = 0;
        node->prev = 0;
    } else {
        tail = list->tail;
        node->prev = tail;
        node->next = 0;
        tail->next = node;
        list->tail = node;
    }

    list->count++;
}

/* 0x080128C0 */
void ListRemove(List *list, ListNode *node)
{
    if (node->next != 0) {
        if (node->prev != 0) {
            node->next->prev = node->prev;
            node->prev->next = node->next;
        } else {
            node->next->prev = node->prev;
            list->head = node->next;
        }
    } else {
        if (node->prev != 0) {
            node->prev->next = node->next;
            list->tail = node->prev;
        } else {
            list->head = 0;
            list->tail = 0;
        }
    }

    list->count--;
}

/* 0x080128FC */
void ListInit(List *list)
{
    list->tail  = 0;
    list->head  = 0;
    list->count = 0;
}

/* 0x08012908 */
void ListForEach(List *list, void (*fn)(ListNode *node))
{
    ListNode *node;
    ListNode *next;

    node = list->head;
    while (node != 0) {
        next = node->next;
        fn(node);
        node = next;
    }
}

/* 0x08012924 */
void ListForEachArg(List *list, void (*fn)(ListNode *node, u32 arg), u32 arg)
{
    ListNode *node;
    ListNode *next;

    node = list->head;
    while (node != 0) {
        next = node->next;
        fn(node, arg);
        node = next;
    }
}
