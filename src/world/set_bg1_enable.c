/* Setting DISPCNT bit 9 — 0x0803084C-0x0803085D
 *
 * ORs the (0x80 << 2) bit into REG_DISPCNT.  Its sibling ClearBg1Enable
 * (0x08030860) is byte-matching; this one sits in its own file because it
 * does not match and leaving it in the same file breaks the sibling's region.
 *
 * Tried (all three 16 bytes): a single local; two locals (`mask = bit`,
 * rule 37 -- agbcc merges the copy); reading DISPCNT first and computing the
 * mask afterwards (did not change the order); making the mask u32.
 *
 * The sibling ClearBg1Enable matched on the FIRST attempt with the SAME
 * address pattern (0x80 << 19); the only difference is that its operation is
 * AND and this one's is OR.  So the obstacle is not in the address setup but
 * in the live range of the mask value.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/set_bg1_enable.c
 */

#include "gba_types.h"

#define DISPCNT  (*(vu16 *)(0x80 << 19))
#define BG1_BIT  (0x80 << 2)

/* 0x0803084C */
void SetBg1Enable(void)
{
    DISPCNT = DISPCNT | BG1_BIT;
}
