/* Clear the low nibble of the +0x8A byte — 0x08028EA4-0x08028EBD
 *
 * The mask is a WIDE -16, built as `movs r0,#16 / negs r0,r0`, so it keeps the
 * high nibble; `~15` on a byte would be a single `movs r0,#240`.
 * src/entry/unlink_and_clear_bit1.c records the same distinction for -2.
 *
 * Rule 33: the mask is materialised first and the byte anded INTO it.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value, and the ROM sets
 * it to 0 after the store.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/clear_low_nibble.c
 */

#include "gba_types.h"

#define BYTE_OFFSET   0x8A
#define KEEP_HIGH     (-16)

typedef struct SlotHolder {
    u8  pad00[0x1C];
    u8 *record;                 /* +0x1C */
} SlotHolder;

extern SlotHolder *GetActiveSlotValue(void);

/* 0x08028EA4 */
u32 FUN_08028ea4(void)
{
    u8 *field = GetActiveSlotValue()->record + BYTE_OFFSET;
    s32 mask;

    mask = KEEP_HIGH;
    mask &= *field;
    *field = mask;
    return 0;
}
