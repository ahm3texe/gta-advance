/* Script command: set flag 0x100000 on the free node — 0x0805AE08-0x0805AE2B
 *
 * Unlike the sibling at 0x0805AB18 this one sets the bit unconditionally; there
 * is no read of the old value to test.
 *
 * Rule 71: the zero answer is at the END here, so the body is the `then` arm.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_set_free_node_flag.c
 */

#include "gba_types.h"

#define FREE_FLAG  (128 << 13)

typedef struct FreeNode {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} FreeNode;

extern FreeNode *FindFreeNode(u16 id);

/* 0x0805AE08 */
u32 FUN_0805ae08(u32 a, u32 id)
{
    FreeNode *node = FindFreeNode(id);
    u32 result;

    if (node != 0) {
        node->flags |= FREE_FLAG;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}
