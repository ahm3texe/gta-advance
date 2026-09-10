/* Collect the ready slots named by the mask — 0x0803C320-0x0803C37B
 *
 * NOT BYTE-MATCHING. 25 of 44 instructions, same length and same structure --
 * every instruction is the ROM's, with two register pairs exchanged: the mask
 * and the result hold r5/r6 the other way round, and the base and the scaled
 * index r0/r1. Four spellings were measured, including an explicit local for
 * the mask assigned before the result, and the pairs never swap.
 *
 * What the file DOES settle is the two tests that look alike and are not.
 *
 * Bit 0 of the mask asks about the first slot table and bit 1 about the second.
 * Each is only consulted when the +0x25 byte of its own 8-byte entry has bit 0
 * set, and the two answers are ored together.
 *
 * The entry's flag is tested with `lsls r0,r1,#31` and the mask with
 * `movs r0,#1 / ands` -- the same question about bit 0, two different shapes,
 * side by side in one function. `(u32)byte << 31` gives the first and
 * `probe = 1; probe &= mask;` the second. A one-bit bitfield through a cast
 * pointer was tried for the first and is much worse (23/48): the cast defeats
 * it and agbcc rebuilds the address each time.
 *
 * Rule 33 for the mask tests: the constant is materialised first and the mask
 * anded into it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/collect_ready_slots.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE  8
#define FLAG_OFFSET   37        /* 0x25, past the entry the index selects */

typedef struct SlotFlags {
    u8 ready : 1;               /* bit 0 */
    u8 rest  : 7;
} SlotFlags;

/* Both symbols keep the types their other users give them
 * (src/world/slot_selectors.c and the slot-query files); the consistency check
 * requires one type per symbol, so each is reached as an address here. */
typedef struct SubData {
    u8  pad00[0x12];
    u16 value;                  /* +0x12 */
} SubData;

typedef struct SlotHead {
    u8       pad00[0x20];
    SubData *sub;               /* +0x20 */
} SlotHead;

extern u32      gRam02001060;
extern SlotHead gRam02000F80;

extern u32 FUN_0803aedc(u32 index, u32 which);

/* 0x0803C320 */
u32 FUN_0803c320(u32 index, u32 mask)
{
    u32 result = 0;
    u32 probe;
    u8 *base;
    u8 *entry;

    probe = 1;
    probe &= mask;
    if (probe != 0) {
        base = (u8 *)&gRam02001060;
        entry = base + index * ENTRY_STRIDE + FLAG_OFFSET;
        if ((u32)*entry << 31)
            result = FUN_0803aedc(index, 1);
    }
    probe = 2;
    probe &= mask;
    if (probe != 0) {
        base = (u8 *)&gRam02000F80;
        entry = base + index * ENTRY_STRIDE + FLAG_OFFSET;
        if ((u32)*entry << 31)
            result |= FUN_0803aedc(index, 2);
    }
    return result;
}
