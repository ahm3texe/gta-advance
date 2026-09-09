/* Rebuilds the sprite list with a stable insertion sort.
 * 0x08012A00-0x08012A97. The node's old next is saved before the insertion.
 * The shared insertion body is unrolled into the loop: 152/152 bytes match.
 * The keys are attr2 & 0x0C00 and node->priority, both in ascending order.
 * Verification: make c-match FILE=src/core/sort_sprite_list.c
 */

#include "sprite_sort.h"

/* 0x08012A00 */
void SortSpriteList(Node **head)
{
    Node *sorted = 0;
    Node *node = *head;
    Node *next;

    while (node != 0) {
        next = node->next;
        InsertSortedSprite(node, &sorted);
        node = next;
    }
    *head = sorted;
}
