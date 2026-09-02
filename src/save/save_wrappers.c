/* Kayit metadata sarmalayicilari — 0x08000BE4-0x08000C27
 *
 * Metadata RAM tamponu 0x02000ED0'da 32 byte; duzeni docs/SAVE_SYSTEM.md
 * icinde. Byte 16 + slot, ilgili slotun gecerli oldugunu gosteren bayraktir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/save_wrappers.c
 */

typedef unsigned char u8;
typedef unsigned int  u32;
typedef signed int    s32;

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
