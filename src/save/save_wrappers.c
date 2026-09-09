/* Save metadata wrappers — 0x08000BE4-0x08000C27
 *
 * The metadata RAM buffer at 0x02000ED0 is 32 bytes; its layout is documented
 * in docs/SAVE_SYSTEM.md. Byte 16 + slot is the validity flag for that slot.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/save_wrappers.c
 */

#include "gba_types.h"

extern u8 gSaveMetadata[32];
#define SAVE_METADATA_SIZE 32
#define SAVE_SLOT_FLAGS    16

extern s32 ReadEepromBytes(u32 offset, s32 size, void *buffer);
extern s32 WriteEepromBytes(u32 offset, s32 size, const void *buffer);

/* 0x08000BE4 */
u8 IsSaveSlotValid(u32 slot)
{
    ReadSaveMetadata(gSaveMetadata);
    return gSaveMetadata[slot + SAVE_SLOT_FLAGS];
}

/* 0x08000C00 */
u32 ReadSaveMetadata(u8 *dest)
{
    ReadEepromBytes(0, SAVE_METADATA_SIZE, dest);
    return 1;
}

/* 0x08000C14 */
u32 WriteSaveMetadata(const u8 *src)
{
    WriteEepromBytes(0, SAVE_METADATA_SIZE, src);
    return 1;
}
