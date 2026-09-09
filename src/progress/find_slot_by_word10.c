/* Index of the slot whose +0x10 pointer matches — 0x080309A8-0x080309D1
 *
 * The sibling of src/progress/find_slot_by_word18.c over the +0x10 field, which
 * docs/GRAM02025810_LAYOUT.md records as a pointer. A null element is skipped
 * before the comparison, so a null argument never matches anything.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/find_slot_by_word10.c
 */

#include "gba_types.h"

#define SLOT_COUNT   24
#define SLOT_STRIDE  180
#define FIELD10      (0x3C + 0x10)

extern u8 gRam02025810[];

/* 0x080309A8 */
s32 FUN_080309a8(u32 wanted)
{
    s32 index = 0;
    u8 *base = gRam02025810;
    u8 *field = base + FIELD10;
    u32 value;

    while (index <= SLOT_COUNT - 1) {
        value = *(u32 *)field;
        if (value != 0) {
            if (value == wanted)
                return index;
        }
        field += SLOT_STRIDE;
        index++;
    }
    return -1;
}
