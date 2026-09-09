/* Shared byte-matching C implementation for sprite insertion and sorting. */
#ifndef GUARD_SPRITE_SORT_H
#define GUARD_SPRITE_SORT_H

#include "sprite_pool.h"

/* The same stable insertion operation is used by the standalone routine
 * at 0x08012C74 and inlined into the loop at 0x08012A00. old_agbcc produces
 * byte-matching output in both cases.
 * A label-based traversal with separate `mask &= field` statements left
 * 30 of 98 bytes differing. A structured for loop combined with direct masks
 * reproduces the ROM's shared 0x0C00 constant and register allocation;
 * neither change was sufficient on its own.
 */
static inline void InsertSortedSprite(Node *node, Node **head)
{
    Node *cur;
    Node *next;
    u32 priority;
    u32 key;
    u32 currentKey;

    priority = node->priority;
    key = node->attr2 & 0x0C00;
    next = *head;
    if (next == 0) {
        *head = node;
        node->next = 0;
        node->prev = 0;
        return;
    }

    cur = next;
    for (;;) {
        currentKey = cur->attr2 & 0x0C00;
        if (currentKey >= key && (currentKey > key || cur->priority > priority)) {
            node->next = cur;
            node->prev = cur->prev;
            if (cur->prev != 0)
                cur->prev->next = node;
            cur->prev = node;
            if (*head == cur)
                *head = node;
            return;
        }
        if (cur->next == 0) {
            cur->next = node;
            node->prev = cur;
            node->next = 0;
            return;
        }
        cur = cur->next;
    }
}

#endif /* GUARD_SPRITE_SORT_H */
