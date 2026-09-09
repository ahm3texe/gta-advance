/* Clear the background scroll and affine transform registers
 * 0x080034EC-0x0800355B  (112 bytes, MATCHED)
 *
 * Without disabling interrupts or making a call, it only writes to the range
 * 0x04000010-0x0400003F: the H/V scroll of the four backgrounds is cleared, the
 * BG2/BG3 affine matrices are set to the identity (pa = pd = 0x100, pb = pc = 0)
 * and their reference points to zero. A leaf function returning with `bx lr`:
 * no stack frame, and a void return type (rule 35).
 *
 * The ROM loads a single literal (0x04000012 = BG0VOFS) and walks to all the
 * others with `adds`/`subs`; that is, the addresses are written as constant
 * expressions in the source and CSE ties them to a single base.
 *
 * THE STORE ORDER WAS READ FROM THE ROM, not guessed (the order is preserved
 * because the accesses are volatile):
 *   1) for each BG, VOFS FIRST then HOFS, from BG0 to BG3
 *   2) the reference points ALTERNATE between BG2/BG3: X low, X high, Y low,
 *      Y high
 *   3) the matrix coefficients likewise alternate between BG2/BG3: pa, pb, pc,
 *      pd
 *
 * THE ONE FORM TRIED MATCHED ON THE FIRST ATTEMPT (112/112). As a note: rule 1
 * DOES NOT APPLY here -- the addresses were not made extern symbols and were
 * left as `(vu16 *)0x040000xx` constant casts. The ROM's single literal plus
 * `adds`/`subs` walk is exactly the signature of a constant expression;
 * converting them to extern symbols would make the base be read separately from
 * the pool (docs/COMPILER.md, "rule 1 is not universal").
 *
 * `volatile` is required (rule 4/12): without the qualifier, agbcc eliminates
 * the consecutive dead stores and empties the function. That AFFINE_ONE is
 * built in THREE instructions (`movs r3,#128 / lsls r3,#1 / adds r2,r3,#0`) --
 * i.e. with an extra register copy -- was not forced from the source; it came
 * out of the allocator by itself.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/cutscene_b1.c
 */

#include "gba_types.h"

/* The scroll registers (write-only). */
#define REG_BG0HOFS (*(vu16 *)0x04000010)
#define REG_BG0VOFS (*(vu16 *)0x04000012)
#define REG_BG1HOFS (*(vu16 *)0x04000014)
#define REG_BG1VOFS (*(vu16 *)0x04000016)
#define REG_BG2HOFS (*(vu16 *)0x04000018)
#define REG_BG2VOFS (*(vu16 *)0x0400001A)
#define REG_BG3HOFS (*(vu16 *)0x0400001C)
#define REG_BG3VOFS (*(vu16 *)0x0400001E)

/* The BG2 affine matrix and reference point. X/Y are 32-bit 28.4 fixed point;
 * because the ROM writes them as two halfwords, low/high are separate here
 * too. */
#define REG_BG2PA   (*(vu16 *)0x04000020)
#define REG_BG2PB   (*(vu16 *)0x04000022)
#define REG_BG2PC   (*(vu16 *)0x04000024)
#define REG_BG2PD   (*(vu16 *)0x04000026)
#define REG_BG2X_L  (*(vu16 *)0x04000028)
#define REG_BG2X_H  (*(vu16 *)0x0400002A)
#define REG_BG2Y_L  (*(vu16 *)0x0400002C)
#define REG_BG2Y_H  (*(vu16 *)0x0400002E)

/* The BG3 affine matrix and reference point. */
#define REG_BG3PA   (*(vu16 *)0x04000030)
#define REG_BG3PB   (*(vu16 *)0x04000032)
#define REG_BG3PC   (*(vu16 *)0x04000034)
#define REG_BG3PD   (*(vu16 *)0x04000036)
#define REG_BG3X_L  (*(vu16 *)0x04000038)
#define REG_BG3X_H  (*(vu16 *)0x0400003A)
#define REG_BG3Y_L  (*(vu16 *)0x0400003C)
#define REG_BG3Y_H  (*(vu16 *)0x0400003E)

/* The 8.8 fixed-point 1.0 value of the identity matrix. */
#define AFFINE_ONE  0x100

/* 0x080034EC */
void ResetBgScrollAndAffine(void)
{
    REG_BG0VOFS = 0;
    REG_BG0HOFS = 0;
    REG_BG1VOFS = 0;
    REG_BG1HOFS = 0;
    REG_BG2VOFS = 0;
    REG_BG2HOFS = 0;
    REG_BG3VOFS = 0;
    REG_BG3HOFS = 0;

    REG_BG2X_L = 0;
    REG_BG3X_L = 0;
    REG_BG2X_H = 0;
    REG_BG3X_H = 0;
    REG_BG2Y_L = 0;
    REG_BG3Y_L = 0;
    REG_BG2Y_H = 0;
    REG_BG3Y_H = 0;

    REG_BG2PA = AFFINE_ONE;
    REG_BG3PA = AFFINE_ONE;
    REG_BG2PB = 0;
    REG_BG3PB = 0;
    REG_BG2PC = 0;
    REG_BG3PC = 0;
    REG_BG2PD = AFFINE_ONE;
    REG_BG3PD = AFFINE_ONE;
}
