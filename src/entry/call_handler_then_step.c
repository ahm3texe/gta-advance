/* Call the +0x08 handler, then step — 0x08035D40-0x08035D5F
 *
 * The sibling of src/entry/call_handler_if_set.c over the +0x08 slot of the
 * same table, with a second call after it. When the handler is absent NEITHER
 * runs.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/call_handler_then_step.c
 */

#include "gba_types.h"

#define TABLE_OFFSET  0xAC

typedef struct Handlers {
    u32   pad00[2];
    void (*step)(void *owner);  /* +0x08 */
} Handlers;

extern void FUN_08035e88(void *owner);

/* 0x08035D40 */
void FUN_08035d40(void *owner)
{
    Handlers *table = *(Handlers **)((u8 *)owner + TABLE_OFFSET);

    if (table->step == 0)
        return;
    table->step(owner);
    FUN_08035e88(owner);
}
