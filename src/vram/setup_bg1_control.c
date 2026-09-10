/* Set up the BG1 control and DISPCNT bits — 0x080125EC-0x08012617
 *
 * BG1CNT gets 128, is read back twice, and then has 0x1F00 ored in; DISPCNT is
 * reached by walking the SAME pointer back 14 bytes and gets 0x800.
 *
 * NOT BYTE-MATCHING. The instruction SEQUENCE is reproduced -- the init store,
 * both read-backs, both or-and-store pairs and the `subs r2,#14` walk all come
 * out in the ROM's order -- but two instructions are missing and the register
 * assignment that follows differs.
 *
 * Rule 74 does its part: the pointer is `volatile`, and without it agbcc folds
 * the walk into a displacement and drops the second read-back, so the shape
 * would not be there at all.
 *
 * WHAT IS LEFT. The ROM copies each mask out of its build register before the
 * or (`movs r3,#248 / lsls r3,#5 / adds r1,r3,#0 / orrs r0,r1`) and ors INTO
 * the value's register; agbcc builds the mask in one register and ors the value
 * into IT. Five spellings were measured -- a plain `|=`, a separate `bits`
 * local, a `mask` local copied into `bits`, the or written as
 * `*reg = value | bits`, and the pointer declared last so its pseudo number
 * changes -- and every one gives the same two-instruction shortfall. This is
 * the register-copy class of rule 44, in the variant rule 75's mixed-type lever
 * does not reach.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/setup_bg1_control.c
 */

#include "gba_types.h"

#define BG1CNT_ADDR   0x0400000E
#define BG1CNT_INIT   128
#define BG1CNT_BITS   (248 << 5)    /* 0x1F00 */
#define DISPCNT_BITS  (128 << 4)    /* 0x0800 */

/* 0x080125EC */
void FUN_080125ec(void)
{
    volatile u16 *reg = (volatile u16 *)BG1CNT_ADDR;
    u16 value;
    u16 bits;

    *reg = BG1CNT_INIT;
    value = *reg;
    *reg = value;
    value = *reg;
    bits = BG1CNT_BITS;
    value |= bits;
    *reg = value;
    reg = (volatile u16 *)((u8 *)reg - 14);
    value = *reg;
    bits = DISPCNT_BITS;
    value |= bits;
    *reg = value;
}
