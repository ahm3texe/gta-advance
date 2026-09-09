/* Slot selectors — 0x0803C49C-0x0803C507
 *
 * Three small selectors use argument 1/2 to choose among slot blocks
 * 0x02000F10, 0x02001140, 0x02001060, and 0x02000F80. The third also extracts
 * u16 +0x12 from the +0x20 substructure when pause flag gGameState[12] is set.
 *
 * Like handler addresses, these pointers are stored in RAM. extern u32 is
 * used instead of #define because the ROM loads them with ldr rather than
 * constructing an address.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_selectors.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SELECT_A     1
#define SELECT_B     2

typedef struct SubData {
    u8 pad00[0x12];
    u16 value;                  /* +0x12 */
} SubData;

typedef struct SlotHead {
    u8       pad00[0x20];
    SubData *sub;               /* +0x20 */
} SlotHead;

extern u32       gRam02001060;
extern SlotHead  gRam02000F80;
extern u8        gGameState[];

/* 0x0803C49C */
void *SelectSlotAB(u32 which)
{
    if (which == SELECT_A)
        return gRam02000F10;
    if (which == SELECT_B)
        return gRam02001140;

    return 0;
}

/* 0x0803C4B8 */
void *SelectSlotCD(u32 which)
{
    if (which == SELECT_A)
        return &gRam02001060;
    if (which == SELECT_B)
        return &gRam02000F80;

    return 0;
}

/* 0x0803C4D4 */
u16 GetSubValue(u32 which)
{
    SlotHead *head;
    SubData  *sub;

    if (which == SELECT_A) {
        head = (SlotHead *)&gRam02001060;
    } else {
        if (gGameState[12] == 0)
            return 0;
        if (which != SELECT_B)
            return 0;
        head = &gRam02000F80;
    }

    sub = head->sub;
    if (sub == 0)
        return 0;

    return sub->value;
}
