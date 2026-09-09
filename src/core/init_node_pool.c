/* Build the node pool — 0x08012AA4-0x08012B1F
 *
 * It threads 128 nodes onto a doubly linked free list, empties the active list,
 * sets the previous frame count to NODE_COUNT and clears OAM with DMA.
 *
 * The first and last nodes are handled OUTSIDE the loop (prev=0 / next=0), with
 * the 126 nodes in between handled in the loop: the ROM's counter starts at 125
 * and loops with `bge`.
 *
 * Rule 43: when the counter and the pointer advance together, both go in the
 * `for` increment, in the ROM's order (ROM: `subs r3,#1` then
 * `adds r2,r1,#0`).
 *
 * The DMA clear is done from a word on the stack with a fixed source (that is
 * what `sub sp,#4` is for); the control word is 0x85000100 = 0x100 words,
 * 32-bit, source fixed.  REG_IME is saved and restored.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/init_node_pool.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

#define OAM_CLEAR_CONTROL 0x85000100

/* 0x08012AA4 */
void InitNodePool(void)
{
    Node *node;
    s32   i;
    u16   ime;
    u32   zero;

    gNodePool.freeHead = &gNodePool.nodes[0];
    gNodePool.activeHead = 0;
    gNodePool.prevCount = NODE_COUNT;

    node = &gNodePool.nodes[0];
    node->next = node + 1;
    node->prev = 0;
    node++;

    for (i = NODE_COUNT - 3; i >= 0; i--, node++) {
        node->prev = node - 1;
        node->next = node + 1;
    }

    node->prev = node - 1;
    node->next = 0;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = &zero;
    REG_DMA3.dst = (void *)OAM_BASE;
    REG_DMA3.control = OAM_CLEAR_CONTROL;
    REG_DMA3.control;
    REG_IME = ime;
}
