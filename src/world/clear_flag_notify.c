/* Clear a flag and notify if empty — 0x08038000-0x0803801F
 *
 * Clear bit 0x8000 at +0x0C. If +0x1C is zero, call ResetActor(0).
 * Rule 35: `pop {r0}; bx r0` indicates a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/clear_flag_notify.c
 */

#include "gba_types.h"

#define BUSY_FLAG 0x8000

typedef struct Node {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
    u8  pad10[12];
    u32 sub;                    /* +0x1C */
} Node;

extern void ResetActor(u32 arg);

/* 0x08038000 */
void ClearFlagNotify(Node *node)
{
    node->flags &= ~BUSY_FLAG;
    if (node->sub == 0)
        ResetActor(0);
}
