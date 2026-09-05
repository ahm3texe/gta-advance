/* Sprite dugumunu sirali listeye ekleme — 0x08012C74-0x08012CD5.
 * Birincil anahtar attr2 & 0x0C00, ikincil anahtar +6 byte'idir.
 * Ikisi de artan sirada; esit anahtarlarin arkasina ekleyerek sirayi korur.
 * Bos liste, araya ekleme ve sona ekleme ROM'da ayri kontrol yollaridir.
 * Dogrulama: make c-match FILE=src/core/insert_sprite_sorted.c
 */

#include "sprite_sort.h"

/* 0x08012C74 */
void InsertSpriteSorted(Node *node, Node **head)
{
    InsertSortedSprite(node, head);
}
