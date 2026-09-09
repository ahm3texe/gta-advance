/* Allocate a node from the free list — 0x08012C0C-0x08012C53
 *
 * It takes a node from the head of the free list and links it onto the head of
 * the active list. The list links are protected with REG_IME (interrupts
 * disabled). If the free list is empty it returns 0.
 *
 * The pool layout was measured from the disassembly:
 *     +0x000  the nodes (2048 bytes = 128 x 16)
 *     +0x800  free list head     -> built with `movs #128 / lsls #4`
 *     +0x804  active list head   -> loaded FROM THE POOL (2052 cannot be built)
 * The two offsets are produced differently because 2048 can be expressed as
 * (8-bit << shift) while 2052 cannot.
 *
 * The base is loaded plainly and the offset built separately -> a STRUCT MEMBER
 * access, not array arithmetic (see the same lesson in
 * flush_palette_queue.c).
 *
 * `pop {r1}; bx r1` with a value in r0 -> it RETURNS A VALUE (the inverse of
 * rule 35).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/alloc_node.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

/* 0x08012C0C */
Node *AllocNode(void)
{
    Node *node;
    Node *old;

    node = gNodePool.freeHead;
    if (node == 0)
        return 0;

    gNodePool.freeHead = node->next;
    node->prev = 0;
    old = gNodePool.activeHead;
    node->next = old;
    REG_IME = 0;
    gNodePool.activeHead = node;
    if (old != 0)
        old->prev = node;
    REG_IME = 1;
    return node;
}
