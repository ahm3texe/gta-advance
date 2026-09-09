/* Setting up the handler and the pack — 0x0803F67C-0x0803F69B
 *
 * Writes the handler pointer to +0x08, copies 12 bytes from the caller to
 * +0x20, puts the value at +0x1C and clears the byte at +0x81.
 *
 * Three rules at once:
 *   - rule 32: a struct assignment produces an ldmia/stmia pair
 *   - rule 37: the ROM takes the destination address into a SEPARATE register
 *     with `adds r3,r0,#32`; the source needs a separate pointer local too
 *   - rule 35: `pop {r0}; bx r0` -> a void return type
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/init_handler_pack.c
 */

#include "gba_types.h"


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

/* The THUMB BIT (bit 0) must be set in a stored function pointer.  The
   address of a symbol with the `__thumb` suffix resolves as | 1
   (tools/agbcc_build.py).  Setting the bit on a `bl` target would corrupt the
   branch offset, so a separate symbol is used for the stored pointer. */
extern u8 FUN_0803d13c__thumb[];

/* 0x0803F67C */
void InitHandlerPack(Slot *slot, Pack12 *src, u32 value)
{
    Pack12 *dest;

    slot->handler = FUN_0803d13c__thumb;
    dest = &slot->pack;
    *dest = *src;
    slot->value = value;
    slot->ready = 0;
}
