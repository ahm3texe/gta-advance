/* Allocating a pool node — 0x08033674-0x080336EB (120 bytes)
 *
 * STATUS: 27/57 instructions, A NEAR MISS (does not match).
 *
 * A node is taken from FUN_08032cc4 (0 if there is none), +0x21 = 16.  The
 * node is looked up in the 8-entry table at gAddrTable+0xB18; if it is not
 * there it is written into the first free slot of the same table (through a
 * SECOND symbol in the ROM, 0x02027330 + 0xB2C!).  Return value:
 * ((node - gAddrTable) / 44) << 24 | (the node's +0 & 0xFFFFFF).
 *
 * THE REMAINING DIFFERENCE IS LAYOUT: the ROM puts the second loop's "free
 * slot found" block (`str r2,[r1]; b tail`) BEFORE the FIRST loop, and it
 * re-reads the node's +0 before each loop.  Tried: two `for`s + goto (27),
 * base locals (27), nested while (15).  No source form that gives that block
 * layout was found; the second search is probably a separate helper or
 * macro.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/alloc_pool_node.c
 */

#include "gba_types.h"
#define NODE_SIZE 44
#define SLOTS     8
typedef struct Node { u32 first; u8 pad04[0x1D]; u8 b21; } Node;
typedef struct AddrEntry {
    u32 addr;                   /* +0x00 */
    u8  pad04[40];              /* stride 44 */
} AddrEntry;
extern AddrEntry gAddrTable[];
extern u8 gRam02027330[];
extern Node *FUN_08032cc4(void);
u32 AllocPoolNode(void)
{
    Node *n; u32 first; s32 i; u32 *slot;
    n = FUN_08032cc4();
    if (n == 0)
        return 0;
    n->b21 = 16;
    first = n->first;
    slot = (u32 *)((u8 *)gAddrTable + 0xB18);
    for (i = 0; i <= SLOTS - 1; i++) {
        if ((Node *)*slot == n)
            goto done;
        slot++;
    }
    first = n->first;
    slot = (u32 *)(gRam02027330 + 0xB2C);
    for (i = 0; i <= SLOTS - 1; i++) {
        if (*slot == 0) {
            *slot = (u32)n;
            break;
        }
        slot++;
    }
done:
    return ((((u8 *)n - (u8 *)gAddrTable) / NODE_SIZE) << 24) | (first & 0xFFFFFF);
}
