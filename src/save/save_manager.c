/* Save manager: slot scanning, loading and writing — 0x08000C28-0x08000DDC
 *
 * The three save slots sit 160 bytes apart in EEPROM. The first 12 bytes of
 * each slot are copied into the gSaveSlotHeaders table in EWRAM. A slot is
 * valid only when its header byte (the marker) plus its complement at byte
 * 156 of the slot sum to 255; a zero marker means the slot is empty.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/save_manager.c
 */

#include "gba_io.h"

/* The inverse direction of rule 1 (as in read/write_eeprom_range.c): DMA3 is
 * written as a constant cast, but as a struct member, because the ROM keeps
 * the base (0x040000D4) in a register and reaches it by offset
 * (`ldr r0, [r2, #8]`). */
#define DMA_ENABLE 0x80000000

/* The EWRAM headers of the three save slots; 12-byte entries. */
typedef struct {
    u8 marker;
    u8 data[11];
} SaveSlotHeader;

#define SAVE_SLOT_COUNT     3
#define SAVE_SLOT_STRIDE  160   /* distance between slots in EEPROM      */
#define SAVE_HEADER_SIZE   12   /* header size kept at the slot start     */
#define SLOT_COMPLEMENT   156   /* the marker's complement is this byte   */
#define MARKER_CHECKSUM   255   /* marker + complement must give this     */
#define EEPROM_DEVICE_TYPE  4   /* type code passed to the ID routine     */
#define SAVE_SETTLE_LOOPS   3   /* short wait after an EEPROM access      */

extern SaveSlotHeader gSaveSlotHeaders[SAVE_SLOT_COUNT];
extern u8 gSaveBuffer[SAVE_SLOT_STRIDE];

/* 0x02000EB8: while nonzero, the EEPROM is identified and access is live. */
extern u32 gEepromAvailable;

/* State words zeroed when the save manager starts up; their roles are not
 * yet resolved in data/ram_map.csv. */
extern u32 gState0;
extern u32 gState1;
extern u8  gState2;
extern u16 gState3;

/* Entry to and exit from the save I/O region (WAITCNT and similar setup);
 * their names are not yet resolved in data/functions.csv. */
extern void StopAudioDmaOnCartFlag(void);
extern void FUN_08033b74(void);

/* EEPROM identification: reports an error with a nonzero u16. */
extern u16 FUN_0806bd34(u32 deviceType);

/* Pre-write preparation, and the value generator for the slot marker. */
extern void FUN_0802fe44(void);
extern u32  FUN_08032548(void);

extern u32 ReadEepromRange(u32 offset, u8 *dest, s32 length);
extern u32 WriteEepromRange(u32 offset, const u8 *src, s32 length);

/* 0x08000C28 — identify the EEPROM, read the three slot headers and validate
 * them.
 *
 * Returns: 1 if the EEPROM is usable, 0 if identification fails.
 *
 * The scanning loop's two byte offsets (into the header table and into the
 * EEPROM) must be explicit local variables. Writing `gSaveSlotHeaders[i]` /
 * `i * SAVE_SLOT_STRIDE` makes agbcc turn both into general induction
 * variables derived from the loop index, and produce a third one
 * (`i * 160 + 156`) that it advances by 160 every step; the ROM has no such
 * third register. With explicit variables no induction variable is produced,
 * and the loop pre-header order matches the ROM's as well (header offset
 * first, then EEPROM offset, then the constant). */
u32 InitSaveManager(void)
{
    u8 complement;            /* single-byte buffer on the stack; the
                                 `volatile` required by rule 3 is not needed
                                 here, both forms give the same bytes */
    u32 available;
    u32 headerOffset;
    u32 offset;
    s32 complementBias;
    int i;

    StopAudioDmaOnCartFlag();

    gState0 = 0;
    gState1 = 0;
    gState2 = 0;
    gState3 = 0;
    REG_IME = 0;

    while (REG_DMA3.control & DMA_ENABLE)
        ;

    gEepromAvailable = 1;
    available = 1;

    /* Rule 8: a forward loop; agbcc turns it into a backwards pointer walk,
     * and that is the form in the ROM. */
    for (i = 0; i < SAVE_SLOT_COUNT; i++)
        gSaveSlotHeaders[i].marker = 0;

    if (FUN_0806bd34(EEPROM_DEVICE_TYPE) != 0) {
        available = 0;
    } else {
        headerOffset = 0;
        offset = 0;
        /* The complement byte's offset is kept negative: the ROM stores the
         * constant in a register as -156 and subtracts (`mov r1, r8` /
         * `sub r0, r5, r1`). Writing `offset + 156` makes agbcc produce two
         * immediate additions and never move the constant into a register. */
        complementBias = -SLOT_COMPLEMENT;

        for (i = 0; i < SAVE_SLOT_COUNT; i++) {
            SaveSlotHeader *header =
                (SaveSlotHeader *)((u8 *)gSaveSlotHeaders + headerOffset);

            if (ReadEepromRange(offset, (u8 *)header, SAVE_HEADER_SIZE) == 0)
                header->marker = 0;
            else if (ReadEepromRange(offset - complementBias,
                                     (u8 *)&complement, 1) == 0)
                header->marker = 0;
            else if (header->marker + complement != MARKER_CHECKSUM)
                header->marker = 0;

            headerOffset += SAVE_HEADER_SIZE;
            offset += SAVE_SLOT_STRIDE;
        }
    }

    gEepromAvailable = 0;
    REG_IME = 1;
    FUN_08033b74();
    return available;
}

/* 0x08000D20 — read a slot into gSaveBuffer and validate its checksum. */
u32 LoadSaveSlot(u32 slot)
{
    SaveSlotHeader *header;
    u32 loaded;
    int i;

    if (slot >= SAVE_SLOT_COUNT)
        return 0;

    /* The inlined form of GetSaveSlotHeader (0x080010D4): a null pointer for
     * an empty slot, then a pointer test. Merging the two stages destroys the
     * `cmp r1, #0` test present in the ROM. */
    if (gSaveSlotHeaders[slot].marker == 0)
        header = 0;
    else
        header = &gSaveSlotHeaders[slot];

    if (header == 0)
        return 0;

    loaded = ReadEepromRange(slot * SAVE_SLOT_STRIDE, gSaveBuffer,
                             SAVE_SLOT_STRIDE);
    if (loaded != 0) {
        if (gSaveBuffer[SLOT_COMPLEMENT] + gSaveBuffer[0] != MARKER_CHECKSUM)
            return 0;
        if (gSaveBuffer[0] == 0)
            return 0;
    }

    for (i = SAVE_SETTLE_LOOPS; i >= 0; i--)
        ;

    return loaded;
}

/* 0x08000D80 — stamps gSaveBuffer with the marker/complement pair and writes it to the slot. */
u32 WriteGameSaveSlot(u32 slot)
{
    SaveSlotHeader *header;
    u32 written;
    u32 marker;
    int i;

    FUN_0802fe44();

    if (slot >= SAVE_SLOT_COUNT)
        return 0;

    /* The marker cannot be zero: zero marks the slot as empty. */
    do {
        marker = FUN_08032548();
        gSaveBuffer[0] = marker;
    } while ((u8)marker == 0);

    gSaveBuffer[SLOT_COMPLEMENT] = ~gSaveBuffer[0];
    /* The inverse direction of rule 2: an intermediate pointer variable is
     * REQUIRED here. Writing `gSaveSlotHeaders[slot] = ...` directly makes
     * agbcc load the base address before the index computation; the ROM
     * builds `slot * 12` first and reads the base afterwards. */
    header = &gSaveSlotHeaders[slot];
    *header = *(SaveSlotHeader *)gSaveBuffer;

    written = WriteEepromRange(slot * SAVE_SLOT_STRIDE, gSaveBuffer,
                               SAVE_SLOT_STRIDE);

    for (i = SAVE_SETTLE_LOOPS; i >= 0; i--)
        ;

    return written;
}
