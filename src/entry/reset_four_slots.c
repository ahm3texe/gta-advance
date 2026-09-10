/* Reset slots 0 to 3 — 0x08019A64-0x08019A7D
 *
 * The sibling of src/entry/reset_five_slots.c: the same loop, but the counter
 * is narrowed to EIGHT bits (`lsls #24 / lsrs #24`), so it is a u8.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/reset_four_slots.c
 */

#include "gba_types.h"

#define SLOT_COUNT  4

extern void FUN_08026f68(u32 slot);

/* 0x08019A64 */
void FUN_08019a64(void)
{
    u8 slot = 0;

    do {
        FUN_08026f68(slot);
        slot = slot + 1;
    } while (slot <= SLOT_COUNT - 1);
}
