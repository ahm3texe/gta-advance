/* Notify if ready — 0x08030884-0x0803089F
 *
 * If the object is not null and its +0x18 field is non-zero, it calls
 * FUN_0802B2B4 with (object, 0, 1).
 *
 * Two rules at once:
 *   - rule 34: a chain of EARLY RETURNS rather than nested `if`s produces the
 *     ROM's block order
 *   - rule 35: `pop {r0}; bx r0` -> a void return type
 *
 * The ROM also copies the argument into a separate register with
 * `adds r1, r0, #0` (rule 37); that does not require a separate local in the
 * source, because the parameter is already used in two roles -- as the query
 * subject and as the call argument.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/notify_if_ready.c
 */

#include "gba_types.h"

typedef struct Node {
    u8  pad00[0x18];
    u32 ready;                  /* +0x18 */
} Node;

extern void FUN_0802b2b4(Node *node, u32 arg1, u32 arg2);

/* 0x08030884 */
void NotifyIfReady(Node *node)
{
    if (node == 0)
        return;
    if (node->ready == 0)
        return;
    FUN_0802b2b4(node, 0, 1);
}
