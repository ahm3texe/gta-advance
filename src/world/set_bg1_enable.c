/* Setting DISPCNT bit 9 — 0x0803084C-0x0803085D
 *
 * BYTE-MATCHING.  It ORs the (0x80 << 2) bit into REG_DISPCNT.  It sits in its
 * own file because it was open long after its sibling ClearBg1Enable
 * (0x08030860) matched, and leaving it beside the sibling breaks that
 * function's region.
 *
 * It stayed open through a search in the wrong direction.  Tried and rejected
 * (all three 16 bytes): a single local; two locals (`mask = bit`, rule 37 --
 * agbcc merges the copy); reading DISPCNT first and computing the mask
 * afterwards (did not change the order); making the mask u32.  The reasoning
 * was that since ClearBg1Enable matched on the FIRST attempt with the SAME
 * address pattern (0x80 << 19), and the only difference is AND versus OR, the
 * obstacle had to be the live range of the mask value.
 *
 * WHAT SOLVED IT: none of those -- the PLAINEST form did, with no local at
 * all, just the expression below.  Every variant above adds something the ROM
 * does not have.
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
