/* Index of the slot whose +0x18 word matches — 0x08030934-0x08030957
 *
 * Walks the 24-element slot array docs/GRAM02025810_LAYOUT.md places at +0x3C
 * with a stride of 180, and answers the index of the first element whose +0x18
 * word equals the argument, or -1.
 *
 * The walking pointer starts at +0x54, which is 0x3C + 0x18: the array base
 * plus the field's offset within an element, folded together. The ROM copies
 * the base out of the pool and adds 84 to the copy, so the two are separate.
 *
 * The bound is a SIGNED comparison (`cmp r1,#23 / ble`), so the index is an int.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/find_slot_by_word18.c
 */

#include "gba_types.h"

#define SLOT_COUNT   24
#define SLOT_STRIDE  180
#define FIELD18      (0x3C + 0x18)

extern u8 gRam02025810[];

/* 0x08030934 */
s32 FUN_08030934(u32 wanted)
{
    s32 index = 0;
    u8 *base = gRam02025810;
    u8 *field = base + FIELD18;

    while (index <= SLOT_COUNT - 1) {
        if (*(u32 *)field == wanted)
            return index;
        field += SLOT_STRIDE;
        index++;
    }
    return -1;
}
