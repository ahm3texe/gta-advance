/* Cizim girisi ayirma — 0x08012E78-0x08012F87
 *
 * Anahtar ROM (0x08000000..0x08FFFFFF), EWRAM (0x02000000..0x0203FFFF) ya
 * da IWRAM (0x03000000..0x03007FFF) icinde ve bayraklar sifir degilse:
 * tur (4,8) ve extra bit15 kuruluysa bayraklar bir kaydirilir. Kuyrukta
 * (gRam02022AB0: +0 sayi, +4 girisler, +8 son indis) FUN_08032434 ile
 * anahtar aranir, bulunursa doner. Yoksa sayi 511'i asmissa kuyruk
 * sifirlanir, gRam0201F2B0 DMA3 ile (0xE00 soz) temizlenir ve
 * LoadEntryTileData cagrilir; ardindan yeni giris (28 bayt) doldurulur,
 * FUN_0803232c ile siralanir, sayi artirilir.
 *
 * UC OLCUM: arama sonucu once `found`a KOPYALANIP sonra sinanmali
 * (sinamadan sonra kopyalanirsa agbcc found=0'i sabit olarak yayip fazla
 * `movs #0 / mov sl` uretiyor); fonksiyonun TEK donus noktasi olmali
 * (`if (e == 0) {...} return e;` -- erken `return found` dali ROM'un
 * ortak `adds r0,r4` cikisini kaciriyor, 1 komut); ayirma blogunda
 * `e->a = e->b = found` (sifir) yazimi ROM'daki r8 kopyasini veriyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/alloc_draw_entry.c
 */

#include "gba_types.h"
#include "gba_io.h"
#define ROM_BASE      0x08000000
#define EWRAM_START   0x02000000
#define IWRAM_START   0x03000000
#define QUEUE_MAX     0x1FF
#define DMA_FILL_CTRL 0x85000E00
typedef struct Entry { u32 a; u32 b; u16 flags; u8 kindA; u8 kindB; u32 key; u8 pad10[12]; } Entry;
typedef struct Queue { u32 count; Entry *entries; s16 last; } Queue;
extern Queue gRam02022AB0;
extern u8    gRam0201F2B0[];
extern Entry *FUN_08032434(Entry *entries, s32 last, u32 key);
extern void   FUN_0803232c(Entry *entries, s16 *last, u32 count);
extern void   LoadEntryTileData(void);
Entry *AllocDrawEntry(u32 key, u32 flags, u8 kindA, u8 kindB, u32 extra)
{
    Queue *q; Entry *found; Entry *e; u16 ime; volatile u32 fill;
    if ((key - ROM_BASE) > 0xFFFFFF && (key - EWRAM_START) > 0x3FFFF && (key - IWRAM_START) > 0x7FFF)
        return 0;
    if (flags == 0)
        return 0;
    if (kindA == 4 && kindB == 8 && (extra & 0x8000))
        flags <<= 1;
    q = &gRam02022AB0;
    e = FUN_08032434(q->entries, q->last, key);
    found = e;
    if (e == 0) {
        if (q->count > QUEUE_MAX) {
            q->count = 0;
            q->last = 0xFFFF;
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = (void *)&fill;
            REG_DMA3.dst = gRam0201F2B0;
            REG_DMA3.control = DMA_FILL_CTRL;
            REG_DMA3.control;
            REG_IME = ime;
            LoadEntryTileData();
        }
        e = &q->entries[q->count];
        e->flags = flags;
        e->kindA = kindA;
        e->kindB = kindB;
        e->key = key;
        e->a = (u32)found;
        e->b = (u32)found;
        FUN_0803232c(q->entries, &q->last, q->count);
        q->count++;
    }
    return e;
}
