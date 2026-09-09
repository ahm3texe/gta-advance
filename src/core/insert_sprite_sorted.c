/* Insert a sprite node into the ordered list — 0x08012C74-0x08012CD5.
 * The primary key is attr2 & 0x0C00 and the secondary key the +6 bytes.
 * Both are ascending; inserting after equal keys preserves the order.
 * An empty list, insertion in the middle and insertion at the end are separate
 * control paths in the ROM.
 * Verification: make c-match FILE=src/core/insert_sprite_sorted.c
 */

#include "sprite_sort.h"

/* 0x08012C74 */
void InsertSpriteSorted(Node *node, Node **head)
{
    InsertSortedSprite(node, head);
}
