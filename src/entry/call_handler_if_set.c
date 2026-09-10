/* Call the +0xAC handler if it is installed — 0x08035D28-0x08035D3F
 *
 * The handler table hangs off +0xAC and the routine is its +0x04 field. The
 * `_call_via_r1` veneer in the ROM is the interworking stub for an indirect
 * call, which agbcc emits for a call through a pointer variable.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/call_handler_if_set.c
 */

#include "gba_types.h"

#define TABLE_OFFSET  0xAC

typedef struct Handlers {
    u32   pad00;
    void (*run)(void *owner);   /* +0x04 */
} Handlers;

/* 0x08035D28 */
void FUN_08035d28(void *owner)
{
    Handlers *table = *(Handlers **)((u8 *)owner + TABLE_OFFSET);

    if (table->run != 0)
        table->run(owner);
}
