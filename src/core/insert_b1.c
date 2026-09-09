/* Set up the background layers — 0x080126E4-0x0801274F
 *
 * Despite the name, this function is NOT a list operation: it sets up the
 * display control and the three background layers (BG0/BG1/BG2), then clears a
 * RAM flag. The struct layouts of the sibling file insert_sorted.c are not used
 * here; the pattern was read entirely from the ROM.
 *
 * What the ROM does (0x080126E4):
 *   DISPCNT = 0x0700            (mode 0; BG0+BG1+BG2 on)
 *   BG0CNT  = 0x0080 ; |= 0x0004 ; |= 0x1E00
 *   BG1CNT  = 0x0002 ; |= 0x0000 ; |= 0x1D00
 *   BG2CNT  = 0x0081 ; |= 0x0004 ; |= 0x1F00
 *   *(u16 *)0x0201AECC = 0
 *
 * MEASURED POINTS
 *
 * 1) THE DISPCNT ADDRESS IS NOT READ FROM THE POOL: the ROM emits
 *    `movs r1,#128; lsls r1,#19`, so the source has a constant cast
 *    (`0x80 << 19`), not an extern symbol. This is the documented exception to
 *    rule 1 (an address that can be built by shifting); the same pattern was
 *    measured in src/world/set_bg1_enable.c.
 *
 * 2) THERE IS NO SEPARATE POOL CONSTANT for BG1CNT and BG2CNT: the ROM loads a
 *    single 0x04000008 literal and does `adds r2, #2` twice. That does NOT mean
 *    there is a walking pointer in the source -- see the elimination note below.
 *
 * 3) Each layer is set up with THREE separate stores: a full assignment first,
 *    then two `|=`. On BG1's first `|=` the ROM has `ldrh`+`strh` but no `orrs`
 *    in between (0x08012712). Because the constant is zero, agbcc eliminates
 *    the `| 0`, while the load and store remain because the access is volatile.
 *    Since the character base block is 0 for BG1, CHAR_BASE(0) was written in
 *    the source.
 *
 * 4) 0x0201AECC was declared as an extern symbol (rule 1). The record was added
 *    to data/ram_map.csv under the name gRam0201AECC; because the only access
 *    in the ROM is a halfword store, the MEASURED size is 2 bytes (the record
 *    says 0, which can be corrected). The record's address comes out directly
 *    as a pool constant, so it is byte-for-byte the same as a constant cast.
 *
 * ELIMINATED PATHS (do not delete; add new ones)
 *
 *   a) A WALKING POINTER (`cnt = BG_CNT_BASE; ... cnt++;`) assigned AFTER the
 *      DISPCNT store: 104 bytes (4 short), 27/54 instructions.
 *      agbcc never loads the pool constant, because the 0x04000000 built for
 *      DISPCNT is still in a register and CSE produces the address more cheaply
 *      as `adds r1, #8`. Every remaining difference stems from the register
 *      shift caused by that single folding.
 *
 *   b) THE SAME WALKING POINTER assigned BEFORE the DISPCNT store: the size
 *      MATCHED (108) but the pool load moved to the TOP of the function
 *      (rule 19: agbcc emits source assignments in source order), 37 bytes of
 *      difference.
 *      So as long as the assignment is a separate statement it either folds or
 *      escapes forward; there is no middle ground.
 *
 *   c) Writing DISPCNT without volatile (`*(u16 *)`): the same as (a), 104
 *      bytes. What prevents the folding is not volatile.
 *
 *   d) THE FORM THAT WORKS: there is NO pointer variable at all; the three
 *      registers are written as THREE SEPARATE constant cast macros. That way
 *      0x04000008 is loaded from the pool AT ITS FIRST USE (it does not escape
 *      forward), and the two neighboring addresses are derived from it with
 *      `adds #2` -- the ROM's exact pattern.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/insert_b1.c
 */

#include "gba_types.h"

/* Display control. The address is built by shifting, not from the pool. */
#define DISPCNT  (*(vu16 *)(0x80 << 19))

/* The background control registers. They must be written separately: a single
 * walking pointer variable does not close the gap ((a) and (b) in the
 * header). */
#define BG0CNT   (*(vu16 *)0x04000008)
#define BG1CNT   (*(vu16 *)0x0400000A)
#define BG2CNT   (*(vu16 *)0x0400000C)

/* Mode 0; the BG0, BG1 and BG2 layers are on. */
#define DISPLAY_MODE  (0xE0 << 3)

/* BGxCNT fields. */
#define PRIORITY(n)    (n)
#define CHAR_BASE(n)   ((n) << 2)
#define SCREEN_BASE(n) ((n) << 8)
#define COLOR_256      0x0080

/* A flag cleared while the layers are set up. A ram_map record is needed. */
extern u16 gRam0201AECC;

/* 0x080126E4 */
void ConfigureBgControlRegs(void)
{
    DISPCNT = DISPLAY_MODE;

    BG0CNT = COLOR_256 | PRIORITY(0);
    BG0CNT |= CHAR_BASE(1);
    BG0CNT |= SCREEN_BASE(30);

    BG1CNT = PRIORITY(2);
    BG1CNT |= CHAR_BASE(0);
    BG1CNT |= SCREEN_BASE(29);

    BG2CNT = COLOR_256 | PRIORITY(1);
    BG2CNT |= CHAR_BASE(1);
    BG2CNT |= SCREEN_BASE(31);

    gRam0201AECC = 0;
}
