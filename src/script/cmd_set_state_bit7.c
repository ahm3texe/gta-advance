/* Script command: set bit 7 in the record's +0x1E state — 0x0805B2BC-0x0805B2E5
 *
 * The same shape as cmd_set_node_flag400.c over a halfword instead of a word:
 * the field is read once, the bit tested, and written back only when it was
 * clear.
 *
 * The field is named twice in the source and agbcc collapses the two reads into
 * the ROM's single `ldrh`. Six spellings were measured: every one that lifted
 * the field into a local first got the structure right but swapped the node and
 * the field between r1 and r2, and none of the usual rule 33 arrangements moved
 * them back. The plain form is the one that matches.
 *
 * The sibling at 0x0805AB18 does want the explicit locals. Its ROM copies the
 * field into a second register before masking (`adds r0,r1,#0 / ands r0,r3`),
 * which is the shape that identifies rule 33; this one materialises the
 * constant instead (`movs r0,#128 / ands r0,r2`), and does not.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_set_state_bit7.c
 */

#include "gba_types.h"

#define STATE_BIT7  128

typedef struct StateNode {
    u8  pad00[0x1E];
    u16 state;                  /* +0x1E */
} StateNode;

extern StateNode *GetOrCreateRecordNode(u16 id);

/* 0x0805B2BC */
u32 FUN_0805b2bc(u32 a, u32 id)
{
    StateNode *node = GetOrCreateRecordNode(id);
    u32 result;

    if (node == 0) {
        result = 0;
    } else {
        if ((node->state & STATE_BIT7) == 0)
            node->state |= STATE_BIT7;
        result = 1;
    }
    return result;
}
