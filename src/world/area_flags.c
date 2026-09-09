/* Area flags — 0x08030C4C-0x08030CB3
 *
 * At offset 0x4C of the record buffer there is a 192-bit (six-word) flag
 * array; the index is ONE-BASED, 0 meaning "none".  src/world/entity_flags.c
 * handles the 128-bit entity flags at 0x3C of the same buffer -- the first 24
 * bytes of the `unk4C[80]` block there are resolved here.
 *
 * The address idiom was measured in entity_flags.c: only a STRUCT MEMBER
 * access produces the `ldr r0,=base / adds r0,#N` form.
 *
 * The fourth function of the same set (CleanupAreaTiles, 0x08030CB4) is in a
 * separate file, src/world/area_cleanup.c; it was open long after these three
 * and now matches as well.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/area_flags.c
 */

#include "gba_types.h"

#define AREA_FLAG_MAX  192

/* The record slot working buffer; for the entity flags at 0x3C see
 * src/world/entity_flags.c. */
typedef struct SaveBuffer {
    u8  header[12];             /* 0x00 */
    u8  unk0C[48];              /* 0x0C */
    u32 entityFlags[4];         /* 0x3C — 128 bits */
    u32 areaFlags[6];           /* 0x4C — 192 bits */
    u8  unk64[56];              /* 0x64 */
    u8  complement;             /* 0x9C */
    u8  unk9D[3];
} SaveBuffer;

extern SaveBuffer gSaveBuffer;

/* 0x08030C4C */
u32 IsAreaFlagSet(s32 index)
{
    s32 word;
    s32 bit;
    u32 mask;

    if (index <= 0)
        return 0;

    index -= 1;
    if (index > AREA_FLAG_MAX - 1)
        return 0;

    bit = index & 31;
    mask = 1 << bit;
    word = index >> 5;
    if ((gSaveBuffer.areaFlags[word] & mask) != 0)
        return 1;

    return 0;
}

/* 0x08030C80 */
u32 SetAreaFlag(s32 index)
{
    s32 word;
    s32 bit;
    u32 mask;

    if (index <= 0)
        return 0;

    index -= 1;
    if (index > AREA_FLAG_MAX - 1)
        return 0;

    bit = index & 31;
    mask = 1 << bit;
    word = index >> 5;
    gSaveBuffer.areaFlags[word] |= mask;

    return 1;
}

/* 0x08030CB0 — its body is empty. */
void AreaFlagsNoop(void)
{
}
