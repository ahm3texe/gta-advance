/* Script command: set flag 0x400 on the record node — 0x0805AB18-0x0805AB43
 *
 * Reads the +0x18 flags once, tests the bit, and writes back only when it was
 * clear. The read-modify-write is not unconditional in the ROM.
 *
 * Rule 33: the ROM copies the flags into a second register and ands the mask
 * into THAT (`adds r0,r1,#0 / ands r0,r3`), leaving the original untouched for
 * the `orrs` that follows. That is a separate probe local, not `flags & MASK`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_set_node_flag400.c
 */

#include "gba_types.h"

#define NODE_FLAG  (128 << 3)

typedef struct RecordNode {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} RecordNode;

extern RecordNode *GetOrCreateRecordNode(u16 id);

/* 0x0805AB18 */
u32 FUN_0805ab18(u32 a, u32 id)
{
    RecordNode *node = GetOrCreateRecordNode(id);
    u32 flags;
    u32 probe;
    u32 result;

    if (node == 0) {
        result = 0;
    } else {
        flags = node->flags;
        probe = flags;
        probe &= NODE_FLAG;
        if (probe == 0) {
            flags |= NODE_FLAG;
            node->flags = flags;
        }
        result = 1;
    }
    return result;
}
