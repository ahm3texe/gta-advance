/* The entity flag query — 0x08032058-0x0803208F
 *
 * BYTE-MATCHING.  The body is 56 bytes: 23 instructions + 2 bytes of alignment
 * padding + a 4-byte literal pool + 2 instructions.  data/functions.csv
 * records 50 bytes for this row; the real body runs up to the next function
 * (GetEntityFixedFields, 0x08032090).
 *
 * The two details that make it match -- both measured, both necessary:
 *
 * 1. The flag words start at byte 60 of the buffer, and the ROM loads the base
 *    SEPARATELY and ADDS 60:
 *      ldr r0,=gSaveBuffer / adds r0,#60 / adds r2,r2,r0
 *    Only a STRUCT MEMBER access produces this form.  With `&gSaveBuffer[60]`
 *    or `(u8 *)gSaveBuffer + 60`, agbcc folds the offset into the literal pool
 *    (`.word gSaveBuffer+0x3c`); a local base pointer instead buries the
 *    offset in the load instruction (`ldr r0,[r0,#0x3c]`).  All three produce
 *    52 bytes.
 *
 * 2. The raw id and the final index must be held in the SAME variable.  With
 *    separate variables agbcc merges `id` and `id - 1` into one register and
 *    produces `subs r0,#1`; the ROM has `subs r0,r3,#1`.  In a single variable
 *    `index`'s lifetime covers the subtraction, the two quantities conflict,
 *    and the register allocation settles onto the ROM's: id/index in r3, the
 *    intermediate in r0.
 *
 * The index shift must also be SIGNED (the ROM has `asrs`) and the bound
 * comparison UNSIGNED (the ROM has `bhi`).
 *
 * Tried and rejected (all measured in the struct-member form):
 *   &gSaveBuffer[60] ............................ 52 bytes / 39 off
 *   a local u8 * base + 60 ...................... 52 bytes / 39 off
 *   an extern u32 array + gSaveBuffer[word + 15]  56 bytes / 17 off
 *   a one-line expression instead of word/bit ... 56 bytes / 21 off
 *   a separate `mask` local ..................... 56 bytes / 22 off
 *   `return (...) != 0;` ........................ 56 bytes / 47 off
 *   a separate `id` local (u16/int/u32) ......... 56 bytes /  8 off
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entity_flags.c
 */

#include "gba_types.h"

typedef struct {
    u8  unk00[22];
    u16 id;                 /* 0x16 — one-based; 0 means "none"        */
} Entity;

/* The record slot working buffer.  Its header and complement fields are known
 * through save_manager.c; the contents of the blocks in between have not been
 * worked out yet.  The entity flags live in four words starting at byte 60. */
typedef struct {
    u8  header[12];         /* 0x00 — marker + slot header               */
    u8  unk0C[48];          /* 0x0C — not worked out                     */
    u32 entityFlags[4];     /* 0x3C — one bit per entity, 128 bits      */
    u8  unk4C[80];          /* 0x4C — not worked out                     */
    u8  complement;         /* 0x9C — the marker's complement            */
    u8  unk9D[3];           /* 0x9D — not worked out                     */
} SaveBuffer;

#define ENTITY_ID_MAX 128

extern SaveBuffer gSaveBuffer;

/* 0x08032058 */
u32 IsEntityFlagSet(const Entity *entity)
{
    s32 index, word, bit;

    /* The raw id and the index share one variable deliberately; see item 2 at
       the top of the file. */
    index = entity->id;
    if (index == 0)
        return 0;

    index = (u16)(index - 1);
    if ((u32)index > ENTITY_ID_MAX - 1)
        return 0;

    word = index >> 5;
    bit = index & 31;
    if (gSaveBuffer.entityFlags[word] & (1 << bit))
        return 1;

    return 0;
}
