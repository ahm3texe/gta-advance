/* Glyph table access — 0x08064698-0x080646AB, 0x080646F8-0x08064723
 *
 * Both index the pointer table at 0x08BD3448 +0x04 with a u16 index:
 * lsls #16 / lsrs #14 simultaneously truncates to u16 and scales by 4.
 *
 * 0x08064698 returns entry +0x10. 0x080646F8 returns 0 if +0x10 is zero,
 * otherwise the ADDRESS of (c << 6) + (b << 4) in the u16 buffer at +0x14.
 * The final lsls #1 scales u16 elements, so the base is u16*.
 * Both return a value in r0 with bx lr and use no stack.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/glyph_table_access.c
 */

#include "gba_types.h"

typedef struct GlyphEntry {
    u8   pad00[16];
    u32  count;                 /* +0x10 */
    u16 *data;                  /* +0x14 */
} GlyphEntry;

typedef struct GlyphRoot {
    u32          pad00;
    GlyphEntry **table;         /* +0x04 */
} GlyphRoot;

extern GlyphRoot gRom08BD3448;

/* 0x08064698 */
u32 GetGlyphCount(u16 index)
{
    return gRom08BD3448.table[index]->count;
}

/* 0x080646F8 */
u16 *GetGlyphCell(u16 index, u32 b, u32 c)
{
    GlyphEntry *entry;
    u16 *base;

    /* ORDER: the ROM loads the data pointer immediately AFTER the branch
 * (ldr r0,[r1,#20]), then computes the offset. &entry->data[...] reversed
 * that order. It also performs no push: this is an r0-r3-only leaf, so
 * avoid extra locals.
 */
    entry = gRom08BD3448.table[index];
    if (entry->count == 0)
        return 0;

    base = entry->data;
    return &base[(c << 6) + (b << 4)];
}
