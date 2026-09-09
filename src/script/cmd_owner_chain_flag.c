/* Script command: flag 0x100 two links down the owner chain
 * 0x08059E80-0x08059EAB
 *
 * The +0x28 field is followed three times: node to owner, owner to its own
 * successor, and once more to reach the word the mask is applied to. Only the
 * first two are checked against zero; the third is not, and the ROM does not
 * check it either.
 *
 * The masked value is the running answer: when the bit is clear the AND has
 * already left zero in the result register, so only the set case needs a store.
 *
 * Three locals in the tail are there for instruction ORDER, not for clarity.
 * The ROM interleaves the first dereference, the mask and the second
 * dereference in that order; folding any of the three back into the expression
 * moves the mask ahead of both loads. Written as one expression the whole tail
 * is one instruction out of place, which is how the arrangement was found.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_owner_chain_flag.c
 */

#include "gba_types.h"

#define CHAIN_FLAG  (128 << 1)

typedef struct ChainLink {
    u8                pad00[0x28];
    struct ChainLink *next;     /* +0x28 */
} ChainLink;

extern ChainLink *FindRecordNodeEnd(u16 id);

/* 0x08059E80 */
u32 FUN_08059e80(u32 a, u32 id)
{
    ChainLink *node = FindRecordNodeEnd(id);
    ChainLink *owner;
    ChainLink *link;
    u32 mask;
    u32 result;

    if (node != 0) {
        owner = node->next;
        if (owner != 0) goto have;
    }
    return 0;
have:
    link = owner->next;
    mask = CHAIN_FLAG;
    result = (u32)link->next & mask;
    if (result != 0)
        result = 1;
    return result;
}
