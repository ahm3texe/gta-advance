/* Kesme sistemi kurulumu — 0x0800038C-0x08000430
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/bootstrap/init_interrupts.c
 */

#include "gba_io.h"

typedef void (*IrqHandler)(void);

/* VBlank + VCount kesmeleri acik, VCount tetigi 50. tarama satirinda. */
#define DISPSTAT_SETUP 0x3228
#define IRQ_ENABLE_MASK 0x0005   /* bit0 VBlank | bit2 VCount */

/* enable | 32 bit birim | 0x200 kelime (2 KB) */
#define DMA_COPY_DISPATCHER 0x84000200

/* IntrMain'in tablo indisleri: ip sayaci 0'dan basliyor ama ilk testten once
 * bir kez artiyor, yani 0 ve 2 hic dagitilmiyor. Tablonun 13 girdisi var,
 * son girdi de dagitim disinda. */
#define IRQ_SLOT_COUNT  13
#define IRQ_SLOT_VBLANK 1
#define IRQ_SLOT_VCOUNT 3

extern IrqHandler gIrqHandlerTable[IRQ_SLOT_COUNT];
extern void *gIrqVector;
/* IRQ dagiticisinin EWRAM'daki kopyasi; BIOS vektoru buraya bakiyor. */
extern u32 gIntrMainEwram[];
/* ram_map.csv'de gEepromAvailable adiyla duruyor; burada IRQ derinlik sayaci
 * olarak sifirlaniyor. Iki rolun ayni sozcugu paylasmasi henuz dogrulanmadi. */
extern u32 gEepromAvailable;
extern u32 gAsyncState;

/* ARM modunda; DMA3 ile EWRAM'a kopyalanip BIOS kesme vektorune baglaniyor. */
extern void IntrMain(void);
extern void DummyIntr(void);
extern void VBlankIntr(void);
extern void VCountIntr(void);

/* Dis semboller assembler'a .equ ile mutlak adres olarak veriliyor
 * (docs/COMPILER.md kural 5), bu yuzden Thumb biti kendiliginden gelmiyor;
 * tabloya yazilan her Thumb isleyicide elle eklenir. */
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

    /* Dagiticiyi EWRAM'a tasirken kesmeler kapali kalir. */
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
