/* Does the chain end's +0x1C match the second operand — 0x08059C48-0x08059C83
 *
 * BOTH lookups take the FIRST operand; the second operand is only the value
 * compared against. The free node's +0x30 must be present, but its value is not
 * used: it is a precondition, not an input.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_chain_end_field1c.c
 */

#include "gba_types.h"

typedef struct ChainNode {
    u8  pad00[0x1C];
    u16 value;                  /* +0x1C */
} ChainNode;

typedef struct FreeNode {
    u8    pad00[0x30];
    void *linked;               /* +0x30 */
} FreeNode;

extern FreeNode  *FindFreeNode(u16 id);
extern ChainNode *FindRecordNodeEnd(u16 id);

/* 0x08059C48 */
u32 FUN_08059c48(u32 a, u16 id, u16 wanted)
{
    FreeNode *node;
    ChainNode *end;
    u32 result;

    node = FindFreeNode(id);
    if (node != 0) {
        if (node->linked != 0) {
            end = FindRecordNodeEnd(id);
            if (end != 0) goto have;
        }
    }
    return 0;
have:
    result = 0;
    if (end->value == wanted)
        result = 1;
    return result;
}
