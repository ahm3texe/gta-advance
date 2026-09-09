/* Release the object held by a slot — 0x08031658-0x08031683
 *
 * Instruction-for-instruction equivalent to ReleaseSlotSub
 * (src/video/blit_strip_plain.c, 0x08031684), with two offset differences:
 * flag at entry +0xB0 (block +0xEC), released field at entry +0x1C
 * (block +0x58). tools/find_twins.py reported 90.9% similarity.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_slot_held.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SLOT_STRIDE  180            /* 0xB4 */
#define OFF_HELD     0x58           /* relative to block; entry +0x1C */
#define OFF_FLAG     0xEC           /* relative to block; entry +0xB0 */

extern void ReleaseObject(u8 *sub);

/* 0x08031658 */
void ReleaseSlotHeld(u32 index)
{
    u8 *base;
    u8 *flag;
    u8 *heldBase;
    u32 scaled;

    base = gRam02025810;
    scaled = index * SLOT_STRIDE;
    flag = base + scaled + OFF_FLAG;
    if (*flag == 0)
        return;

    heldBase = base + OFF_HELD;
    ReleaseObject(heldBase + scaled);
    *flag = 0;
}
