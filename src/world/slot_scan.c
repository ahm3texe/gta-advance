/* Slot scanner and DMA clear — 0x08055C8C-0x08055CFD
 *
 * The first finds the count-th empty slot (word +0x28 == 0) in a 128-entry
 * array of 60-byte slots and returns its pointer. The second clears slots
 * with DMA3 32-bit transfers, 15 words (60 bytes) per slot.
 *
 * DMA control 0x85000000 supplies the 32-bit transfer flags; low 16 bits
 * hold the length.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_scan.c
 */

#include "gba_io.h"

#define SLOT_COUNT     128
#define SLOT_STRIDE    60
#define DMA_XFER_32    0x85000000
#define WORDS_PER_SLOT 15

typedef struct Slot {
    u8  pad00[0x28];
    u32 mark;                   /* +0x28: zero means empty */
    u8  pad2C[0x10];            /* stride 60 */
} Slot;

extern Slot gSlotArray[SLOT_COUNT];

/* 0x08055C8C — pointer to the count-th empty slot, or 0. */
Slot *FindNthFreeSlot(u32 count)
{
    Slot *slot;
    Slot *found;
    u32   seen;
    s32   i;

    if (count == 0)
        return 0;

    slot  = gSlotArray;
    found = gSlotArray;
    seen  = 0;

    for (i = 0; i < SLOT_COUNT; i++, slot++) {
        if (slot->mark != 0) {
            seen = 0;
            found = slot + 1;
        } else {
            seen++;
            if (seen == count)
                return found;
        }
    }

    return 0;
}

/* 0x08055CC8 — DMA zero-fill count slots at dest (15*count words). */
void ClearSlots(void *dest, u32 count)
{
    volatile u32 fill;
    u16 ime;

    if (count == 0)
        return;

    ime = REG_IME;
    REG_IME = 0;

    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = dest;
    REG_DMA3.control = DMA_XFER_32 | (WORDS_PER_SLOT * count);
    REG_DMA3.control;

    REG_IME = ime;
}
