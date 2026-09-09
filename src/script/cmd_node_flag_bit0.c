/* Script command: bit 0 of the chain end's +0x1A flags — 0x08059C84-0x08059CA1
 *
 * Resolves the operand to a node with FindRecordNodeEnd and answers bit 0 of
 * the node's +0x1A halfword, or 0 when there is no such node.
 *
 * Rule 33: the ROM materialises the constant first and ands the field into it
 * (`movs r0,#1 / ldrh r1,[r1,#26] / ands r0,r1`), so the 1 is a separate local
 * that the flags are anded INTO. Written `1 & node->flags` the result would
 * stay in the flags' register instead.
 *
 * agbcc emits a two-armed if in source order: the `then` arm falls through, the
 * `else` arm goes after it. Here the ROM tests the node against zero and lands
 * on the flag read, so the flag read is the `then` arm and the zero the `else`.
 * The actor-flag siblings a few hundred bytes further on are the mirror image.
 * An early `return` instead of an explicit else does not give either layout
 * reliably; both were measured.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_node_flag_bit0.c
 */

#include "gba_types.h"

typedef struct ChainNode {
    u8  pad00[0x1A];
    u16 flags;                  /* +0x1A */
} ChainNode;

extern ChainNode *FindRecordNodeEnd(u16 id);

/* 0x08059C84 */
u32 FUN_08059c84(u32 a, u32 id)
{
    ChainNode *node = FindRecordNodeEnd(id);
    u32 bit;

    if (node != 0) {
        bit = 1;
        bit &= node->flags;
    } else {
        bit = 0;
    }
    return bit;
}
