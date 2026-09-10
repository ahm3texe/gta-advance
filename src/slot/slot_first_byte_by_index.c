/* The first byte of a slot's record, by index — 0x0803C3D4-0x0803C3FF
 *
 * Index 1 takes the first slot; index 2 takes the third but only while byte 12
 * of gGameState is set. Anything else answers 0.
 *
 * The zero answer stands at the very END, after the shared tail, so both
 * failures are `goto`s and not early returns; written as early returns the
 * zeros come out before the tail and the whole middle inverts.
 *
 * Both arms converge on ONE pair of loads, `ldr r0,[r0,#28] / ldrb r0,[r0,#0]`,
 * so the two slot pointers are read the same way and the source shares that
 * tail rather than writing it twice.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/slot_first_byte_by_index.c
 */

#include "gba_types.h"

#define TWO_PLAYER  12          /* gGameState byte 12 */

typedef struct SubData {
    u8  pad00[0x12];
    u16 value;                  /* +0x12 */
} SubData;

typedef struct SlotHead {
    u8       pad00[0x20];
    SubData *sub;               /* +0x20 */
} SlotHead;

typedef struct SlotRecord {
    u8  pad00[0x1C];
    u8 *first;                  /* +0x1C */
} SlotRecord;

extern u32      gRam02001060;
extern SlotHead gRam02000F80;
extern u8       gGameState[];

/* 0x0803C3D4 */
u32 FUN_0803c3d4(u32 index)
{
    SlotRecord *slot;

    if (index == 1) {
        slot = (SlotRecord *)&gRam02001060;
        goto have;
    }
    if (index != 2) goto zero;
    if (gGameState[TWO_PLAYER] == 0) goto zero;
    slot = (SlotRecord *)&gRam02000F80;
have:
    return *slot->first;
zero:
    return 0;
}
