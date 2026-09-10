/* Run FUN_080148B0 down the +0x44 chain — 0x080150B4-0x080150CD
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/step_all_on_chain.c
 */

#include "gba_types.h"

typedef struct ChainNode {
    u8                pad00[0x44];
    struct ChainNode *next;     /* +0x44 */
} ChainNode;

extern void FUN_080148b0(ChainNode *node);

/* 0x080150B4 */
void FUN_080150b4(ChainNode *node)
{
    if (node == 0)
        return;
    do {
        FUN_080148b0(node);
        node = node->next;
    } while (node != 0);
}
