/* Kayit serilestirme yardimcilari — 0x080010F8-0x0800114B
 *
 * Sekiz fonksiyonun yedisi C'den byte-matching; WriteU16LE acik. Blok
 * tamamlanana kadar src/save/save_helpers.s gecerli build kaynagidir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/save_helpers.c
 */

#include "gba_types.h"

/* Uc kayit slotunun EWRAM'daki basligi; 12 byte'lik girisler. */
typedef struct {
    u8 marker;
    u8 data[11];
} SaveSlotHeader;

#define SAVE_SLOT_COUNT  3
#define SAVE_SLOT_STRIDE 160

extern SaveSlotHeader gSaveSlotHeaders[SAVE_SLOT_COUNT];

extern void *Memset(void *dest, int value, u32 count);
extern u32 WriteEepromRange(u32 offset, const void *src, u32 size);

/* 0x08001094 — HENUZ ESLESMIYOR (62 byte'in 15'i farkli)
 *
 * Yapi ve tum komutlar dogru; fark taban adresin ne zaman yuklendiginde.
 * ROM once tabani yukluyor, sonra indeksi hesapliyor:
 *     ldr r1, =0x02000460 ; lsls r0, r4, #1 ; adds r0, r0, r4 ; lsls r0, r0, #2
 * agbcc tersini yapiyor: once indeks, sonra ldr. Ayni fark
 * GetSaveSlotHeader'da da var, yani sistematik. */
u32 EraseSaveSlot(u32 slot)
{
    u8 buffer[8];

    if (slot >= SAVE_SLOT_COUNT)
        return 0;

    gSaveSlotHeaders[slot].marker = 0;
    Memset(buffer, 0, sizeof(buffer));
    return WriteEepromRange(slot * SAVE_SLOT_STRIDE, buffer, sizeof(buffer));
}

/* 0x080010D4 — HENUZ ESLESMIYOR (36 byte'in 11'i farkli)
 *
 * Sekiz komutun tamami dogru; sira ve register dagitimi farkli:
 *     ROM   : adds r2, r0, #0 ... ldr r0, [pc] ; lsls r1, r2, #1 ; ...
 *     agbcc : adds r1, r0, #0 ... lsls r0, r1, #1 ; ... ldr r2, [pc]
 *
 * Denenip TUTMAYANLAR: yerel degisken sayisi ve sirasi (bes farkli dizilim),
 * isaretci aritmetigi vs dizi indeksleme, tabani ayri degiskene alma,
 * ters kosul (if(marker) return h), void* donus tipi, extern dizi sembolu
 * yerine sabit cast ve tersi, agbcc yerine old_agbcc ve tersi.
 * Bes dizilimin BESI de byte-byte ayni ciktiyi verdi: agbcc bu fonksiyonda
 * C bicimine duyarsiz. Kalan olasi sebep derleyici surumu veya bulunmamis
 * bir bayrak; C'yi kurcalayarak cozulecek gibi gorunmuyor. */
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
 * Parametre ISARETLI dar tip: ROM giriste degeri 16 bite normalize ediyor
 * (lsls #16 / lsrs #16). Bu cift, `u16` parametrede hic uretilmez -- cunku
 * isaretsiz HImode parametre cagirandan zaten sifir-genisletilmis gelir.
 * `s16` yazilinca deger isaret-genisletilmis kabul edilir ve agbcc kullanmadan
 * once ust yariyi temizlemek zorunda kalir; ROM'daki cift tam olarak budur.
 * Yani bu dort bayt, ozgun kaynakta parametrenin isaretli oldugunun kanitidir.
 * Kardes fonksiyonlar isaretsiz kalir (WriteU8 -> u8, WriteU32LE -> u32);
 * WriteU8'in ROM'da 4 byte olmasi, yani normalizasyon icermemesi bunu
 * dogruluyor. */
void WriteU16LE(u8 *p, s16 v)
{
    p[0] = v;
    p[1] = v >> 8;
}

/* 0x08001130 — byte-matching
 * Maskeyi literal havuzdan okumak yerine iki kez mov+lsl ile yeniden kurmasi
 * agbcc'ye ozgu; 28 byte'in tamami birebir. */
void WriteU32LE(u8 *p, u32 v)
{
    p[0] = v;
    p[1] = (v & 0xff00) >> 8;
    p[2] = (v & 0xff0000) >> 16;
    p[3] = v >> 24;
}
