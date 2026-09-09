/* Per-frame walk of the three node lists — 0x08053D48-0x08053DF7
 *
 * StepCountdownTimers is called first (it does not use the r0 argument it
 * receives and loads its own base; the ROM still sets the argument up:
 * 0x02035760).
 * Then THREE separate linked lists are walked with the same pattern:
 *
 *     node = *list_head;
 *     while node != 0 and node->id != 0x7FEF:
 *         if node->kind > 0, the list's own handler is called
 *         node = node->next
 *
 * 0x7FEF is the sentinel id marking the end of the list. The ROM reads it from
 * the pool once and keeps it in r5 (a loop invariant), while doing the first
 * comparison against the freshly loaded r0: this is agbcc's rotated form of
 * `for (n = head; n && n->id != SENTINEL; ...)`.
 *
 * NoOp0805AB78 is called at the end.
 *
 * The node layout (measured from the ROM):
 *     +0x00  struct Node *next        ldr rX,[rY,#0]
 *     +0x08  u16 id                   ldrh rX,[rY,#8]
 *     +0x0B  the upper nibble, a SIGNED 4-bit field
 *
 * The field at +0x0B produces `ldrb / lsls #24 / asrs #28`: that is exactly the
 * read of a SIGNED 4-bit bitfield starting at bit 4 within the byte (the signed
 * form of rule 24). A bitfield was declared rather than writing masks.
 *
 * `pop {r4,r5}; pop {r0}; bx r0` -> a void return type (rule 35).
 *
 * AN EXISTING TYPE WAS REUSED: the same layout as the Node in
 * src/world/node_search.c (+0 next, +8 id); because the +0x0B field is needed
 * here too, the full layout was written file-locally rather than defining a
 * second type under the same name. The gRam... names match those in
 * data/ram_map.csv exactly.
 *
 * MATCH: 176/176 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c4.c
 */

#include "gba_types.h"
#include "node_list.h"

/* The block immediately before the 0x02035780 list head; StepCountdownTimers
   clears it.  It was not verified as a separate symbol, hence the address. */
#define NODE_BLOCK_BASE ((void *)0x02035760)

#define NODE_SENTINEL_ID 0x7FEF


/* There is no symbol for 0x02035A80 in ram_map; a raw address cast was used. */
#define gNodeListC (*(NodeC4 **)0x02035A80)

extern NodeC4 *gNodeListHead;       /* 0x02035A70 */

extern void StepCountdownTimers(void *arg);
extern void FUN_080534a8(NodeC4 *node);
extern void AllocateNodeSlots(NodeC4 *node);
extern void FUN_08052228(NodeC4 *node);
extern void NoOp0805AB78(void);

/* 0x08053D48 */
void StepNodeLists(void)
{
    NodeC4 *node;

    StepCountdownTimers(NODE_BLOCK_BASE);

    for (node = gRam02035780;
         node != 0 && node->id != NODE_SENTINEL_ID;
         node = node->next) {
        if (node->kind > 0)
            FUN_080534a8(node);
    }

    for (node = gNodeListHead;
         node != 0 && node->id != NODE_SENTINEL_ID;
         node = node->next) {
        if (node->kind > 0)
            AllocateNodeSlots(node);
    }

    for (node = gNodeListC;
         node != 0 && node->id != NODE_SENTINEL_ID;
         node = node->next) {
        if (node->kind > 0)
            FUN_08052228(node);
    }

    NoOp0805AB78();
}
