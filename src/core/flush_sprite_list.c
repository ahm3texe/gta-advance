/* Write the active node list into OAM — 0x08012B9C-0x08012C0B
 *
 * It walks the active list from the front, writing each node's three OAM
 * attributes, then disables the surplus slots left over FROM THE PREVIOUS FRAME
 * and stores the new count back.
 *
 * The OAM base is built with `movs r3,#224 / lsls r3,#19` = 0x07000000.
 * The empty slot pattern: attr0=512 (0x200, OBJ disabled), attr1=0, attr2=0.
 *
 * The pointer advances 2/2/4: an OAM entry is 8 bytes, but the fourth halfword
 * (the transform data) is not written and is skipped.
 *
 * The pool layout GREW with this function:
 *     +0x800  free list head
 *     +0x804  active list head
 *     +0x808  previous frame count  <- found HERE; the ram_map size was
 *                                      corrected from 2056 to 2060
 *
 * The shared layout of the node and the pool is in include/sprite_pool.h.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * MATCH: 112/112 bytes. The remaining 9-byte instruction-order difference was
 * closed by `for (i = count; i < left; i++)`. When agbcc converts an ascending
 * induction variable into a descending counter, it places the `left - count`
 * subtraction AFTER the loop constants. Writing `left -= count` by hand instead
 * emits the subtraction as a source statement, BEFORE the constants.
 * Separate constant locals and `while (--left != count)` attempts did not
 * match.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/flush_sprite_list.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

#define OBJ_HIDDEN 512

/* 0x08012B9C */
void FlushSpriteList(void)
{
    Node *node;
    vu16 *oam;
    s32   count;
    s32   left;
    s32   i;

    count = 0;
    oam = (vu16 *)OAM_BASE;
    node = gNodePool.activeHead;
    if (node != 0) {
        do {
            *oam = node->attr0;
            oam++;
            *oam = node->attr1;
            oam++;
            *oam = node->attr2;
            oam += 2;
            count++;
            node = node->next;
        } while (node != 0);
    }

    left = gNodePool.prevCount;
    if (count < left) {
        for (i = count; i < left; i++) {
            *oam = OBJ_HIDDEN;
            oam++;
            *oam = 0;
            oam++;
            *oam = 0;
            oam += 2;
        }
    }
    gNodePool.prevCount = count;
}
