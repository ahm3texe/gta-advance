/* Take the node off the list and clear bit 0 — 0x080358B4-0x080358D3
 *
 * The mask is -2, built as `movs r0,#2 / negs r0,r0`, so it clears bit 0 and
 * not bit 1. Written `~2` the mask is 0xFD as a byte and agbcc emits a single
 * `movs r0,#253`; the negation is what makes it a wide -2.
 *
 * Rule 33: the mask is materialised first and the +0x0A byte anded INTO it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/unlink_and_clear_bit1.c
 */

#include "gba_types.h"

#define CLEAR_BIT0  (-2)

typedef struct Node {
    u8 pad00[10];
    u8 flags;                   /* +0x0A */
} Node;

typedef struct List {
    Node *head;
    Node *tail;
    int   count;
} List;

extern List gList02027EF0;

extern void ListRemove(List *list, Node *node);

/* 0x080358B4 */
void FUN_080358b4(Node *node)
{
    s32 mask;

    ListRemove(&gList02027EF0, node);
    mask = CLEAR_BIT0;
    mask &= node->flags;
    node->flags = mask;
}
