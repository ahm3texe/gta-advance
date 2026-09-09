/* Release a slot — 0x080308AC-0x080308E3
 *
 * gRam02025810 +0x4C holds 24 slots of 180 bytes: +0 object pointer, +4
 * (block +0x50) auxiliary field. Releasing clears 0x02000000 in object +0x0C.
 *
 * BYTE-MATCHING. One scaled = index * 180 expression with separate heldBase
 * and extraBase locals preserves the ROM's single r3 offset added to two bases.
 *
 * Matching sibling: src/world/slot_release.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_slot.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SLOT_MAX        23
#define SLOT_STRIDE     180
#define FLAG_CLEAR      0xFDFFFFFF      /* ~0x02000000 */

typedef struct Held {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Held;

/* 0x080308AC */
u32 ReleaseSlot(u32 index)
{
    u8    *base;
    u8    *heldBase;
    u8    *extraBase;
    Held **heldSlot;
    u32    scaled;
    u32   *extraSlot;
    Held  *held;

    if (index <= SLOT_MAX) {
        base = gRam02025810;
        scaled = index * SLOT_STRIDE;
        heldBase = base + 0x4C;
        heldSlot = (Held **)(heldBase + scaled);
        held = *heldSlot;
        if (held != 0)
            held->flags &= FLAG_CLEAR;

        extraBase = base + 0x50;
        extraSlot = (u32 *)(extraBase + scaled);
        *extraSlot = 0;
        *heldSlot = 0;
    }

    return 1;
}
