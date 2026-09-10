/* Is the active slot's +0xB1 byte 0xFF — 0x08028E88-0x08028EA1
 *
 * The +0xB1 read goes through a pointer add because Thumb's ldrb immediate
 * reaches only 31.
 *
 * Rule 73: the 0 is written after the label, so the 1 falls through first,
 * which is the layout the ROM has.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/is_active_byte_unset.c
 */

#include "gba_types.h"

#define BYTE_OFFSET  0xB1
#define UNSET        0xFF

typedef struct SlotHolder {
    u8  pad00[0x1C];
    u8 *record;                 /* +0x1C */
} SlotHolder;

extern SlotHolder *GetActiveSlotValue(void);

/* 0x08028E88 */
u32 FUN_08028e88(void)
{
    u8 *record = GetActiveSlotValue()->record;

    if (record[BYTE_OFFSET] != UNSET) goto no;
    return 1;
no:
    return 0;
}
