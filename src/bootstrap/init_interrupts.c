/* Interrupt system setup — 0x0800038C-0x08000430
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/bootstrap/init_interrupts.c
 */

#include "gba_io.h"

typedef void (*IrqHandler)(void);

/* VBlank + VCount interrupts enabled, the VCount trigger on scanline 50. */
#define DISPSTAT_SETUP 0x3228
#define IRQ_ENABLE_MASK 0x0005   /* bit0 VBlank | bit2 VCount */

/* enable | 32-bit unit | 0x200 words (2 KB) */
#define DMA_COPY_DISPATCHER 0x84000200

/* IntrMain's table indices: the ip counter starts at 0 but is incremented
 * once before the first test, so 0 and 2 are never dispatched. The table has
 * 13 entries, and the last one is outside dispatch as well. */
#define IRQ_SLOT_COUNT  13
#define IRQ_SLOT_VBLANK 1
#define IRQ_SLOT_VCOUNT 3

extern IrqHandler gIrqHandlerTable[IRQ_SLOT_COUNT];
extern void *gIrqVector;
/* The EWRAM copy of the IRQ dispatcher; the BIOS vector points here. */
extern u32 gIntrMainEwram[];
/* It appears under the name gEepromAvailable in ram_map.csv; here it is
 * cleared as the IRQ depth counter. That the two roles share the same word is
 * not verified yet. */
extern u32 gEepromAvailable;
extern u32 gAsyncState;

/* In ARM mode; copied to EWRAM with DMA3 and hooked into the BIOS interrupt
 * vector. */
extern void IntrMain(void);
extern void DummyIntr(void);
extern void VBlankIntr(void);
extern void VCountIntr(void);

/* External symbols are given to the assembler as absolute addresses with .equ
 * (docs/COMPILER.md rule 5), so the Thumb bit does not come along by itself;
 * it is added by hand for every Thumb handler written into the table. */
#define THUMB_ENTRY(fn) ((IrqHandler)((u32)(fn) + 1))

/* 0x0800038C */
void InitInterrupts(void)
{
    u16 saved_ime;

    REG_IME = 0;

    gIrqHandlerTable[0] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[IRQ_SLOT_VBLANK] = THUMB_ENTRY(VBlankIntr);
    gIrqHandlerTable[2] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[IRQ_SLOT_VCOUNT] = THUMB_ENTRY(VCountIntr);
    gIrqHandlerTable[4] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[5] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[6] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[7] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[8] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[9] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[10] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[11] = THUMB_ENTRY(DummyIntr);
    gIrqHandlerTable[12] = THUMB_ENTRY(DummyIntr);

    /* Interrupts stay disabled while the dispatcher is moved to EWRAM. */
    saved_ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = (const void *)IntrMain;
    REG_DMA3.dst = gIntrMainEwram;
    REG_DMA3.control = DMA_COPY_DISPATCHER;
    REG_DMA3.control;
    REG_IME = saved_ime;

    gIrqVector = gIntrMainEwram;
    gEepromAvailable = 0;
    gAsyncState = 0;

    REG_DISPSTAT = DISPSTAT_SETUP;
    REG_IE = IRQ_ENABLE_MASK;
    REG_IME = 1;
}
