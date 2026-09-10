/* Reset slots 0 to 4 — 0x08019800-0x08019819
 *
 * The counter is narrowed to 16 bits after each step (`lsls #16 / lsrs #16`),
 * so it is a u16, and the bound is UNSIGNED (`bls`).
 *
 * The sibling at 0x08019A64 is the same loop over four slots with an 8-bit
 * counter; the two narrowings are what tell the widths apart.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/reset_five_slots.c
 */

#include "gba_types.h"

#define SLOT_COUNT  5

extern void FUN_08028144(u32 slot);

/* 0x08019800 */
void FUN_08019800(void)
{
    u16 slot = 0;

    do {
        FUN_08028144(slot);
        slot = slot + 1;
    } while (slot <= SLOT_COUNT - 1);
}
