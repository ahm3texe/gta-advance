/* Slot release caller — 0x080308A0-0x080308AB
 *
 * One-line forwarding call; FUN_080308EC does the work. Its matching sibling
 * ReleaseSlot is separate: src/world/release_slot.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_release.c
 */

#include "gba_types.h"


extern void FUN_080308ec(void);

/* 0x080308A0 */
void ReleaseAllSlots(void)
{
    FUN_080308ec();
}
