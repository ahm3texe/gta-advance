/* Save system initialization — 0x0800082C-0x0800091B
 *
 * Initializes the EEPROM library, clamps the slot count to the range 1..16,
 * validates or creates the CRAWSAVE metadata signature, and aligns the
 * per-slot save size to eight bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  python3 tools/verify_c_function.py src/save/init_save_system.c
 */

#include "gba_io.h"

#define SAVE_SLOT_MIN     1
#define SAVE_SLOT_MAX     16
#define SAVE_SLOT_FLAGS   16
#define EEPROM_TOTAL      480
#define EEPROM_BLOCK_MASK 7

extern u8  gSaveMetadata[32];
extern s32 gSavePayloadSize;
extern s32 gSaveSlotCount;

extern void FUN_0806bd34(s32 mode);
extern s32  __divsi3(s32 dividend, s32 divisor);
extern u32  ReadSaveMetadata(u8 *dest);
extern u32  WriteSaveMetadata(const u8 *src);

/* 0x0800082C — 240/240 bytes BYTE-MATCHING
 *
 * Three details of the source form were found by measurement; all three are
 * required:
 *
 *  1. The flag-clearing loop is written FORWARDS (COMPILER.md rule 8). agbcc
 *     reverses it itself and turns it into the downward-walking pointer in the
 *     ROM (adds r2,#31 / subs r2,#1, counter 15..0). Writing it backwards by
 *     hand produces different code -- the measured first-difference offsets:
 *       i=0..15,  [16+i] forwards  -> @176  (the loop matches EXACTLY)
 *       i=15..0,  [16+i] backwards -> @152
 *       i=16..31, [i]    forwards  -> @112
 *       i=0..15,  [31-i]           -> @160
 *
 *  2. The divisor argument of the division call is taken into a local variable
 *     FIRST (COMPILER.md rule 11). The ROM builds argument 2 first
 *     (ldr r0,=&gSaveSlotCount / ldr r1,[r0]), then the constant argument 1
 *     (movs r0,#240 / lsls r0,#1). Writing gSaveSlotCount directly into the
 *     call makes agbcc build the constant first and the order comes out
 *     reversed (23 differences).
 *
 *  3. TWO SEPARATE pointer variables are needed for &gSavePayloadSize, each
 *     used only ONCE. The ROM keeps the address in callee-saved r4 and copies
 *     it into r3 inside the if block (adds r3,r4,#0). Using a single pointer
 *     in two places lets agbcc hoist the &gSavePayloadSize computation to the
 *     top of the function: the function shrinks to 236 bytes and the first
 *     difference moves back to offset 48. Using a second local variable for
 *     the same address brings the copy back.
 *     Every pointer-free form (the global used directly) stays at a
 *     41-byte difference. */
s32 InitSaveSystem(s32 slotCount)
{
    s32 size;
    s32 i;
    s32 slots;
    s32 *payloadSize;

    REG_IME = 0;
    FUN_0806bd34(4);
    REG_IME = 1;

    gSaveSlotCount = slotCount;
    if (slotCount <= 0)
        gSaveSlotCount = SAVE_SLOT_MIN;
    else if (slotCount > SAVE_SLOT_MAX - 1)
        gSaveSlotCount = SAVE_SLOT_MAX;

    ReadSaveMetadata(gSaveMetadata);

    if (gSaveMetadata[0] != 'C' || gSaveMetadata[1] != 'R'
        || gSaveMetadata[2] != 'A' || gSaveMetadata[3] != 'W'
        || gSaveMetadata[4] != 'S' || gSaveMetadata[5] != 'A'
        || gSaveMetadata[6] != 'V' || gSaveMetadata[7] != 'E'
        || gSaveMetadata[8] != (u8)gSaveSlotCount) {
        gSaveMetadata[0] = 'C';
        gSaveMetadata[1] = 'R';
        gSaveMetadata[2] = 'A';
        gSaveMetadata[3] = 'W';
        gSaveMetadata[4] = 'S';
        gSaveMetadata[5] = 'A';
        gSaveMetadata[6] = 'V';
        gSaveMetadata[7] = 'E';
        gSaveMetadata[8] = gSaveSlotCount;

        /* Clear the slot flags -- written forwards, see (1) above */
        for (i = 0; i < SAVE_SLOT_MAX; i++)
            gSaveMetadata[SAVE_SLOT_FLAGS + i] = 0;

        WriteSaveMetadata(gSaveMetadata);
    }

    /* The address read on return; it lives in a register across the calls */
    payloadSize = &gSavePayloadSize;

    slots = gSaveSlotCount;
    size = __divsi3(EEPROM_TOTAL, slots);
    gSavePayloadSize = size;

    /* Align the per-slot size down to eight bytes */
    if (size & EEPROM_BLOCK_MASK) {
        s32 *alignedSize = &gSavePayloadSize;

        do {
            size--;
        } while (size & EEPROM_BLOCK_MASK);

        *alignedSize = size;
    }

    return *payloadSize;
}
