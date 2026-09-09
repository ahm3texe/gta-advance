/* Initialize the sprite pool — 0x08012B20-0x08012B9B.
 * It threads 128 nodes onto a doubly linked free list and empties the active
 * list. prevCount=128 makes the first FlushSpriteList hide every unused OAM
 * slot. DMA3 then clears the 1024 bytes of OAM; the previous REG_IME value is
 * saved across the DMA and restored.
 * The `for` increment in the order `i++, node++` emits the ROM's counter
 * decrement before the pointer copy; with node++ in the body there was a
 * 4-byte ordering difference.
 * Verification: make c-match FILE=src/core/init_sprite_pool.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

/* 0x08012B20 */
void InitSpritePool(void)
{
    Node *node;
    s32 i;
    u16 ime;
    volatile u32 zero;

    gNodePool.freeHead = gNodePool.nodes;
    gNodePool.activeHead = 0;
    gNodePool.prevCount = NODE_COUNT;

    node = gNodePool.nodes;
    node->next = node + 1;
    node->prev = 0;
    node++;
    for (i = 1; i < NODE_COUNT - 1; i++, node++) {
        node->prev = node - 1;
        node->next = node + 1;
    }
    node->prev = node - 1;
    node->next = 0;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = (void *)OAM_BASE;
    REG_DMA3.control = 0x85000100;
    (void)REG_DMA3.control;
    REG_IME = ime;
}
