/* Script command: reset an area node — 0x0805A2F4-0x0805A317
 *
 * Finds or creates the area node for the operand, clears it through
 * FUN_08032008, writes 5 into its +0x16 byte and then clears slot 0.
 *
 * The node pointer stays in r4 across three calls, which is what the
 * `push {r4}` pays for.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_area_reset.c
 */

#include "gba_types.h"

typedef struct AreaNode {
    u8  pad00[0x16];
    u8  kind;                   /* +0x16 */
} AreaNode;

extern AreaNode *FindOrInitAreaNode(u16 id);

extern void FUN_08032008(AreaNode *node, u32 value);
extern void SetSlot(u32 value);

/* 0x0805A2F4 */
u32 FUN_0805a2f4(u32 a, u32 id)
{
    AreaNode *node = FindOrInitAreaNode(id);

    FUN_08032008(node, 0);
    node->kind = 5;
    SetSlot(0);
    return 1;
}
