/* Glif tablosu erisimi — 0x08064698-0x080646AB, 0x080646F8-0x08064723
 *
 * Iki fonksiyon da 0x08BD3448 + 0x04'teki isaretci tablosunu u16 indeksle
 * okuyor: `lsls #16 / lsrs #14` kalibi hem u16'ya kirpip hem 4 ile
 * carpiyor (isaretci dizisi indeksi).
 *
 * 0x08064698: girisin +0x10 alanini donduruyor.
 * 0x080646F8: +0x10 sifirsa 0 donuyor, degilse +0x14'teki u16 tamponunda
 *   (c << 6) + (b << 4) konumunun ADRESINI donduruyor.  Sondaki `lsls #1`
 *   u16 olceklemesi, yani taban u16*.
 *
 * Ikisi de `bx lr` ile donuyor (yigin kullanmiyor) ve r0'da deger tasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/glyph_table_access.c
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

    /* SIRA: ROM veri isaretcisini daldan HEMEN SONRA yukluyor
       (ldr r0,[r1,#20]), ofseti sonra hesapliyor.  `&entry->data[...]`
       yazmak tersini uretiyordu.  Ayrica ROM push YAPMIYOR: sadece
       r0-r3 kullanan bir yaprak, o yuzden fazladan yerel tutmamak
       gerekiyor. */
    entry = gRom08BD3448.table[index];
    if (entry->count == 0)
        return 0;

    base = entry->data;
    return &base[(c << 6) + (b << 4)];
}
