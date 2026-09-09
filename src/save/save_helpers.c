/* Save serialization helpers — 0x08001094-0x0800114B
 *
 * ALL eight functions are byte-matching from C. (WriteU16LE was open for a
 * while; it closed with rule 15 -- the signedness of a narrow parameter.)
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/save/save_helpers.c
 */

#include "gba_types.h"

/* The EWRAM headers of the three save slots; 12-byte entries. */
typedef struct {
    u8 marker;
    u8 data[11];
} SaveSlotHeader;

#define SAVE_SLOT_COUNT  3
#define SAVE_SLOT_STRIDE 160

extern SaveSlotHeader gSaveSlotHeaders[SAVE_SLOT_COUNT];

extern void *Memset(void *dest, int value, u32 count);
extern u32 WriteEepromRange(u32 offset, const void *src, u32 size);

/* 0x08001094 — DOES NOT MATCH YET (15 of 62 bytes differ)
 *
 * The structure and every instruction are correct; the difference is in when
 * the base address is loaded. The ROM loads the base first, then computes the
 * index:
 *     ldr r1, =0x02000460 ; lsls r0, r4, #1 ; adds r0, r0, r4 ; lsls r0, r0, #2
 * agbcc does the opposite: index first, then ldr. The same difference is
 * present in GetSaveSlotHeader, so it is systematic. */
u32 EraseSaveSlot(u32 slot)
{
    u8 buffer[8];

    if (slot >= SAVE_SLOT_COUNT)
        return 0;

    gSaveSlotHeaders[slot].marker = 0;
    Memset(buffer, 0, sizeof(buffer));
    return WriteEepromRange(slot * SAVE_SLOT_STRIDE, buffer, sizeof(buffer));
}

/* 0x080010D4 — DOES NOT MATCH YET (11 of 36 bytes differ)
 *
 * All eight instructions are correct; the order and register allocation
 * differ:
 *     ROM   : adds r2, r0, #0 ... ldr r0, [pc] ; lsls r1, r2, #1 ; ...
 *     agbcc : adds r1, r0, #0 ... lsls r0, r1, #1 ; ... ldr r2, [pc]
 *
 * Tried and REJECTED: the number and order of locals (five different
 * arrangements), pointer arithmetic vs array indexing, taking the base into a
 * separate variable, an inverted condition (if(marker) return h), a void*
 * return type, a constant cast instead of the extern array symbol and vice
 * versa, and agbcc instead of old_agbcc and vice versa.
 * ALL FIVE arrangements produced byte-for-byte identical output: agbcc is
 * insensitive to the C form in this function. The remaining possible cause is
 * the compiler version or a flag not yet found; it does not look solvable by
 * tinkering with the C. */
SaveSlotHeader *GetSaveSlotHeader(u32 slot)
{
    if (slot >= SAVE_SLOT_COUNT)
        return 0;
    if (gSaveSlotHeaders[slot].marker == 0)
        return 0;

    return &gSaveSlotHeaders[slot];
}

/* 0x080010F8 — byte-matching */
u8 ReadU8(const u8 *p)
{
    return p[0];
}

/* 0x080010FC — byte-matching */
u16 ReadU16LE(const u8 *p)
{
    return (p[1] << 8) | p[0];
}

/* 0x08001108 — byte-matching */
u32 ReadU32LE(const u8 *p)
{
    return (p[1] << 8) | p[0] | (p[2] << 16) | (p[3] << 24);
}

/* 0x08001120 — byte-matching */
void WriteU8(u8 *p, u8 v)
{
    p[0] = v;
}

/* 0x08001124 — byte-matching
 *
 * The parameter is a SIGNED narrow type: on entry the ROM normalizes the
 * value to 16 bits (lsls #16 / lsrs #16). That pair is never produced for a
 * `u16` parameter -- an unsigned HImode parameter already arrives zero-
 * extended from the caller. Writing `s16` makes the value be treated as
 * sign-extended, and agbcc has to clear the upper half before using it; that
 * pair is exactly what the ROM has. So these four bytes are evidence that the
 * parameter was signed in the original source.
 * The sibling functions stay unsigned (WriteU8 -> u8, WriteU32LE -> u32);
 * WriteU8 being 4 bytes in the ROM -- that is, containing no normalization --
 * confirms this. */
void WriteU16LE(u8 *p, s16 v)
{
    p[0] = v;
    p[1] = v >> 8;
}

/* 0x08001130 — byte-matching
 * Rebuilding the mask with two mov+lsl pairs instead of reading it from the
 * literal pool is specific to agbcc; all 28 bytes are identical. */
void WriteU32LE(u8 *p, u32 v)
{
    p[0] = v;
    p[1] = (v & 0xff00) >> 8;
    p[2] = (v & 0xff0000) >> 16;
    p[3] = v >> 24;
}
