/* Shutting the hardware down and soft-resetting — 0x08065650-0x080656F3
 *
 * Clears the control field of the four DMA channels in two steps (first the
 * repeat/timing bits, then the enable bit), READING once after each channel --
 * the dummy read needed for the write to settle in hardware, which is why the
 * READ view must be volatile.  It then zeroes the mixer, interrupt, display
 * and four background control registers and calls the BIOS soft reset.
 *
 * STATUS: BYTE-MATCHING — 164/164 bytes (0x08065650-0x080656F3).
 *
 * Two levers closed it (both candidates for new rules):
 *
 * 1) On the DMA control register the WRITE view must NOT be volatile while the
 *    READ view must stay volatile.  Writing to a `volatile` lvalue, agbcc
 *    emits a dead `ldrh` right before the `strh`; across four channels that
 *    meant eight extra instructions (16 bytes).  Two views of the same address
 *    (DMAn_W plain for writing, REG_DMAn volatile for reads and dummy reads)
 *    took the difference from 139 to 4 bytes.  The same class as rule 57's
 *    SIOMLT_SEND row; verified here for DMA control as well.
 *
 * 2) `REG_IME = zero = 0;` as a CHAINED assignment.  A separate `zero = 0;`
 *    statement produces the constant first (`movs r5,#0 / ldr r0,=IME`),
 *    whereas the ROM loads the address first (`ldr r0,=IME / movs r5,#0 /
 *    strh`).  In a chained assignment the constant is formed as the RIGHT-hand
 *    side of the outer assignment -- that is, AFTER the LHS address is
 *    produced -- and the ROM's order comes out.  That closed the last 4 bytes.
 *
 * SPELLINGS RULED OUT: an explicit read-and-write instead of `&=` (no change),
 * binding the dummy read to a variable (no change), writing all the zeroes as
 * plain constants (168 bytes / 137 off -- the zero is not kept in r5 but
 * regenerated at every write), keeping the zero in a single long-lived local
 * initialized by a separate statement (139 off).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/shutdown_reset.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define DMA_CLEAR_TIMING 0xC5FF   /* the repeat, gamepak and timing bits */
#define DMA_CLEAR_ENABLE 0x7FFF   /* the enable bit */
#define RESET_ALL        0xFF

/* The write view is NOT volatile (rule 57): a volatile lvalue produces a dead
 * `ldrh` before the write.  The read and the dummy read stay volatile. */
#define DMA0_W (*(DmaRegs *)0x040000B0)
#define DMA1_W (*(DmaRegs *)0x040000BC)
#define DMA2_W (*(DmaRegs *)0x040000C8)
#define DMA3_W (*(DmaRegs *)0x040000D4)

extern void SoftResetSystem(u32 flags);

/* 0x08065650 */
void ShutdownAndReset(void)
{
    u16 zero;

    REG_IME = zero = 0;

    DMA0_W.control = REG_DMA0.control & DMA_CLEAR_TIMING;
    DMA0_W.control = REG_DMA0.control & DMA_CLEAR_ENABLE;
    REG_DMA0.control;

    DMA1_W.control = REG_DMA1.control & DMA_CLEAR_TIMING;
    DMA1_W.control = REG_DMA1.control & DMA_CLEAR_ENABLE;
    REG_DMA1.control;

    DMA2_W.control = REG_DMA2.control & DMA_CLEAR_TIMING;
    DMA2_W.control = REG_DMA2.control & DMA_CLEAR_ENABLE;
    REG_DMA2.control;

    DMA3_W.control = REG_DMA3H.control & DMA_CLEAR_TIMING;
    DMA3_W.control = REG_DMA3H.control & DMA_CLEAR_ENABLE;
    REG_DMA3H.control;

    REG_BLDCNT   = zero;
    REG_BLDY     = zero;
    REG_IE       = zero;
    REG_DISPSTAT = zero;
    REG_BG0CNT   = zero;
    REG_BG1CNT   = zero;
    REG_BG2CNT   = zero;
    REG_BG3CNT   = zero;

    SoftResetSystem(RESET_ALL);
}
