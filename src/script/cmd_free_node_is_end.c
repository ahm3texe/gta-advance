/* Is the free node's +0x30 the other operand's chain end — 0x08059B58-0x08059B8F
 *
 * Two operands, each resolved by a different lookup, and the answer is whether
 * the first one's +0x30 word IS the node the second resolves to. A failure of
 * either lookup answers 0.
 *
 * The zero answer sits BETWEEN the two tests and the comparison, with the
 * second test branching over it, so the first test is written as a wrapper `if`
 * and the second as a `goto` out of it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_free_node_is_end.c
 */

#include "gba_types.h"

typedef struct ChainNode ChainNode;

typedef struct FreeNode {
    u8         pad00[0x30];
    ChainNode *linked;          /* +0x30 */
} FreeNode;

extern FreeNode  *FindFreeNode(u16 id);
extern ChainNode *FindRecordNodeEnd(u16 id);

/* 0x08059B58 */
u32 FUN_08059b58(u32 a, u16 id, u16 other)
{
    FreeNode *node = FindFreeNode(id);
    ChainNode *end;
    u32 result;

    if (node != 0) {
        end = FindRecordNodeEnd(other);
        if (end != 0) goto have;
    }
    return 0;
have:
    result = 0;
    if (node->linked == end)
        result = 1;
    return result;
}
