/* Sprite listesini kararli eklemeli siralama ile yeniden kurar.
 * 0x08012A00-0x08012A97. Dugumun eski next'i eklemeden once saklanir.
 * Ortak ekleme govdesi dongunun icine acilir: 152/152 bayt eslesir.
 * Anahtarlar attr2 & 0x0C00 ve node->priority, ikisi de artan sirada.
 * Dogrulama: make c-match FILE=src/core/sort_sprite_list.c
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
