/* Script command: is the area node ready — 0x0805A318-0x0805A339
 *
 * Answers 1 when IsEntityFlagSet says so for the node's +0x18 entry, and
 * otherwise falls back to the node's own +0x17 byte.
 *
 * Four spellings were measured. A two-armed if with a result variable (either
 * way round) hoists the constant 1 above the test and routes both answers
 * through r1; the ROM keeps everything in r0. Of the two `goto` forms, only
 * this one matches, and note which way round it is: the body written AFTER the
 * label is the one that ends up FIRST in the ROM, reached by falling through.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_area_ready.c
 */

#include "gba_types.h"

typedef struct AreaEntry AreaEntry;

typedef struct AreaNode {
    u8         pad00[0x17];
    u8         ready;           /* +0x17 */
    AreaEntry *entry;           /* +0x18 */
} AreaNode;

extern AreaNode *FindOrInitAreaNode(u16 id);

extern s32 IsEntityFlagSet(AreaEntry *entry);

/* 0x0805A318 */
u32 FUN_0805a318(u32 a, u32 id)
{
    AreaNode *node = FindOrInitAreaNode(id);

    if (IsEntityFlagSet(node->entry) == 0) goto fallback;
    return 1;
fallback:
    return node->ready;
}
