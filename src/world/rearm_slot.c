/* Re-arming the slot — 0x0803F69C-0x0803F6D7
 *
 * Sets one bit and clears another in the first object's +0x0C flags; if the
 * handler differs from the expected one it installs it and zeroes +0x32;
 * writes the value to +0x2C and clears +0x81.
 *
 * Three rules at once:
 *   - rule 37: the ROM copies both `slot` (r3) and the incoming value (r4)
 *     into SEPARATE registers; neither lives across a call, but both live
 *     across multiple uses
 *   - a symbol with the `__thumb` suffix: bit 0 must be set in a stored or
 *     compared function pointer (tools/agbcc_build.py)
 *   - rule 35: `pop {r0}; bx r0` -> a void return type
 *
 * The Slot definition must be IDENTICAL to the one in
 * src/world/init_handler_pack.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/rearm_slot.c
 */

#include "gba_types.h"

#define FLAG_SET   (0x80 << 16)
#define FLAG_CLEAR 0x01000000

typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

typedef struct Slot {
    void  *first;               /* +0x00 */
    u8     pad04[4];
    void  *handler;             /* +0x08 */
    u8     pad0C[16];
    u32    value;               /* +0x1C */
    Pack12 pack;                /* +0x20 */
    u32    unk2C;               /* +0x2C */
    u8     pad30[2];
    u16    unk32;               /* +0x32 */
    u8     pad34[77];
    u8     ready;               /* +0x81 */
} Slot;

typedef struct Obj {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Obj;

extern u8 FUN_0803d208__thumb[];

/* 0x0803F69C */
void RearmSlot(Slot *slot, u32 value)
{
    Obj *obj;

    obj = (Obj *)slot->first;
    obj->flags = (obj->flags | FLAG_SET) & ~FLAG_CLEAR;

    if (slot->handler != FUN_0803d208__thumb) {
        slot->handler = FUN_0803d208__thumb;
        slot->unk32 = 0;
    }

    slot->unk2C = value;
    slot->ready = 0;
}
