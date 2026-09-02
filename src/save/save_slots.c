/* Genel kayit slotu okuma/yazma — 0x08000B00-0x08000BE3
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/save_slots.c
 */

typedef unsigned char u8;
typedef unsigned int  u32;
typedef signed int    s32;

#define SAVE_SLOT_FLAGS   16
#define EEPROM_BLOCK_SIZE 8
#define SLOT_DATA_BLOCK   4

extern u8  gSaveMetadata[32];
extern s32 gSavePayloadSize;
extern s32 gSaveSlotCount;

extern s32  ReadEepromBytes(s32 block, s32 size, void *buffer);
extern s32  WriteEepromBytes(s32 block, s32 size, const void *buffer);
extern u32  ReadSaveMetadata(u8 *dest);
extern u32  WriteSaveMetadata(const u8 *src);

/* 0x08000B00 */
u32 WriteSaveSlot(s32 slot, const void *src, u32 length)
{
    s32 count;
    s32 size;

    count = gSaveSlotCount;
    if (count == 0)
        return 0;

    size = gSavePayloadSize;
    if (size == 0)
        return 0;

    if (slot < 0)
        slot = 0;
    else if (slot >= count)
        slot = count - 1;

    if (length == 0 || length > gSavePayloadSize)
        length = gSavePayloadSize;

    WriteEepromBytes(gSavePayloadSize / EEPROM_BLOCK_SIZE * slot + SLOT_DATA_BLOCK,
                     length, src);

    ReadSaveMetadata(gSaveMetadata);
    gSaveMetadata[slot + SAVE_SLOT_FLAGS] = 1;
    WriteSaveMetadata(gSaveMetadata);
    return 1;
}

/* 0x08000B78 */
u32 ReadSaveSlot(s32 slot, void *dest, u32 length)
{
    ReadSaveMetadata(gSaveMetadata);
    if (gSaveMetadata[slot + SAVE_SLOT_FLAGS] == 0)
        return 0;

    if (slot < 0)
        slot = 0;
    else if (slot >= gSaveSlotCount)
        slot = gSaveSlotCount - 1;

    if (length == 0 || length > gSavePayloadSize)
        length = gSavePayloadSize;

    ReadEepromBytes(gSavePayloadSize / EEPROM_BLOCK_SIZE * slot + SLOT_DATA_BLOCK,
                    length, dest);
    return 1;
}
