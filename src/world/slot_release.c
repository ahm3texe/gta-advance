/* Yuva serbest birakma cagricisi — 0x080308A0-0x080308AB
 *
 * Tek satirlik devretme. Asil is FUN_080308EC'de.
 * Kardesi ReleaseSlot (0x080308AC) henuz eslesmedigi icin ayri dosyada:
 * src/world/release_slot.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_release.c
 */

#include "gba_types.h"


extern void FUN_080308ec(void);

/* 0x080308A0 */
void ReleaseAllSlots(void)
{
    FUN_080308ec();
}
