/* Sprite ekleme ve liste siralama icin ortak, C'den eslesen govde. */
#ifndef GUARD_SPRITE_SORT_H
#define GUARD_SPRITE_SORT_H

#include "sprite_pool.h"

/* 0x08012C74'te bagimsiz, 0x08012A00'da dongu icine acilmis ayni
 * kararli ekleme islemi. old_agbcc her iki bicimi byte-matching uretir.
 * Etiketli dolasim + ayri mask &= field deyimleri 30/98 fark birakti.
 * Yapisal for dongusu + dogrudan maskeler birlikte ROM'un paylasilan
 * 0x0C00 sabitini ve register dagitimini uretir; tek baslarina yetmezler.
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
