/* Slot table — 0x08055D08-0x08055D8F
 *
 * 160-entry u16 ID table at 0x02030C10; 0x7FEF means empty. The third
 * function DMA-fills a region with this empty value.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_table.c
 */

#include "gba_io.h"

#define ID_NONE        0x7FEF
#define SLOT_COUNT     160
#define ENTRY_SIZE     32
#define ENTRY_SHIFT    5
#define DMA_FILL_16    0x81000000

typedef struct EntryTable {
    u8  pad00[0x28];
    u8 *entries;                /* +0x28 */
} EntryTable;

#define ENTRY_TABLE  ((const EntryTable *)0x08D49C00)

extern u16 gSlotIds[SLOT_COUNT];

/* 0x08055D08 */
u8 *GetEntrySlot(int index)
{
    return ENTRY_TABLE->entries + (index << ENTRY_SHIFT);
}

/* 0x08055D18 — find count consecutive empty IDs and return the run's start. */
u16 *FindFreeSlotRun(int count)
{
    int run;
    u16 *slot;
    u16 *start;
    int i;

    if (count == 0)
        return 0;

    slot = gSlotIds;
    start = slot;
    run = 0;

    for (i = 0; i <= SLOT_COUNT - 1; i++, slot++) {
        if (*slot == ID_NONE) {
            run++;
            if (run == count)
                return start;
        } else {
            run = 0;
            start = slot + 1;
        }
    }

    return 0;
}

/* 0x08055D54 */
void FillSlotsWithNone(void *dest, u32 count)
{
    volatile u16 fill;
    u16 ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = ID_NONE;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = dest;
    REG_DMA3.control = DMA_FILL_16 | count;
    REG_DMA3.control;
    REG_IME = ime;
}
