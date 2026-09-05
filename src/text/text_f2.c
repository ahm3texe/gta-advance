/* Tek glifi metin katmanina yerlestirme — 0x08064020-0x0806417F
 *
 * draw_text.c'deki DrawTextAt her karakter icin bunu cagiriyor. Gorevi:
 * kirpma testleri, karakter normalizasyonu ve glif karolarinin hedef
 * adreslerini kurup asil piksel karistiricisini (BlendGlyphAcrossTiles, text_f1.c)
 * cagirmak.
 *
 * Akis:
 *   1. Ekran disi kirpmalari: x > 239, y > 159, y < 0, x <= -16.
 *   2. 146 -> 39 ikamesi (draw_text.c'deki GlyphAdvance ile ayni kalip),
 *      sonra _toupper. Glif tablosu yalnizca buyuk harf tasiyor.
 *   3. Genislik ONCEDEN aliniyor (GetGlyphWidth), cunku hem cikis testinde
 *      hem de asagida "ikinci karo sutunu gerekiyor mu" kararinda lazim.
 *      ROM bu degeri yigin yuvasinda tutuyor (sub sp,#4) -- derleyicinin
 *      kendi tasmasi, kaynakta karsiligi yok.
 *   4. 95 ('_') tamamen atlaniyor; 32 altindaki kontrol karakterleri de.
 *   5. x TEK ise bir artiriliyor: hedef yarim-kelime iki pikseli birlikte
 *      tasidigi icin glif ciftlere hizali olmak zorunda.
 *   6. x + genislik <= 0 ise glif tumuyle solda kalmis demektir.
 *
 * Yerlesim: satir = y >> 3, sutun = x >> 3, karo ici piksel = x & 7
 * (ROM bunu `x - (sutun << 3)` olarak yaziyor, maskeyle degil).
 * Hedef bayt adresi = gTextVramBase + sutun*64 + satir*gTextRowStride*64.
 *
 * IKI GLIF DUZENI -- ayrimi gHalfLineSpacing yapiyor:
 *   gHalfLineSpacing != 0  : 8x8 glif, karo basina 64 bayt, TEK cagri.
 *   gHalfLineSpacing == 0  : 16x16 glif, 256 bayt, DORT karo. Tablodaki
 *     siralama +0x00 sol-ust, +0x40 sag-ust, +0x80 sol-alt, +0xC0 sag-alt.
 *     Alt karo bir satir asagi (dest + stride*64). Sag sutun yalnizca
 *     genislik 8'i asiyorsa ciziliyor; o zaman x 8 artirilip sutun ve
 *     karo ici piksel yeniden hesaplaniyor.
 * clear_text_area.c ayni bayragi ayni yonde okuyor (0 -> tam satir
 * yuksekligi), yani yorum tutarli.
 *
 * gTextRowStride her cagridan SONRA yeniden okunuyor: cagri bellegi
 * bozabildigi icin agbcc onu onbellege alamiyor. Adresi ise sl'de
 * tutuluyor -- bu, kaynakta ayri bir yerel degil, sabit havuzundan tek
 * yukleme yapmasinin dogal sonucu.
 *
 * 0x080640CA-0x080640DB ve 0x08064174-0x0806417F araliklari KOD DEGIL,
 * sabit havuzu (dort RAM adresi). Disassembler oralari komut sanir.
 *
 * ESLESMEYI SAGLAYAN IKI SEY (olculdu, 356/352 baytlik ilk surumden geldi):
 *
 * 1) ILK PARAMETRE KELIME GENISLIGINDE, `u8` DEGIL (kural 15).
 *    `u8 ch` yazilinca agbcc prologa giris normalizasyonu (lsls #24 /
 *    lsrs #24) koyuyor; ROM'da o iki komut YOK, dogrudan `adds r4,r0,#0`
 *    var. `u8` -> 311/356 fark, kelime genisligi -> 141/352. Ayrica
 *    normalizasyonun kullandigi register tum dagitimi bir kaydiriyordu.
 *    NOT: clear_text_area.c ve draw_text.c bu fonksiyonu hala
 *    `extern void PlaceGlyph(u8 ch, s32 x, s32 y);` diye bildiriyor.
 *    Cagri ABI'si ayni (deger zaten r0'da u8 olarak geliyor) ve o iki
 *    dosya BYTE-MATCHING durumda, ama bildirim ile tanim tip olarak
 *    ayrisiyor; bir sonraki dokunusta oradaki bildirimler `s32`ye
 *    cekilmeli.
 *
 * 2) HAM KARAKTER VE TABLO INDEKSI TEK DEGISKEN (kural 27).
 *    Ayri bir `index` yereli tutmak `adds r3,r0,#0` + `adds r4,r3,#0`
 *    ciftini ve x/y'nin r5<->r6 takasini uretiyordu. `ch`'yi yerinde
 *    donusturmek (once _toupper, sonra `ch -= 32`) her ikisini de
 *    kapatti: 141 -> 0.
 *
 * DENENIP ELENEN: `u8 ch` imzasi (311/356) ve ayri `index` yereli
 * (141/352). Uc surumde eslesme saglandigi icin register dagitimina
 * yonelik baska bir hamleye (volatile, ayri taban yerelleri, kisa omurlu
 * gecici bloklar) gerek kalmadi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/text_f2.c
 */

#include "gba_types.h"

#define GLYPH_FIRST      32   /* tablonun ilk karakteri (bosluk) */
#define GLYPH_SUBSTITUTE 146  /* metinde gecen ozel bayt */
#define GLYPH_APOSTROPHE 39   /* onun karsiligi */
#define GLYPH_SKIP       95   /* '_' hic cizilmiyor */
#define SCREEN_RIGHT     239
#define SCREEN_BOTTOM    159
#define LEFT_LIMIT       16   /* x <= -16 ise glif tamamen solda */
#define TILE_SHIFT       3    /* 8 piksel = 1 karo */
#define TILE_ORDER       6    /* 8bpp karo = 64 bayt */
#define GLYPH_ORDER      8    /* 16x16 glif = 256 bayt */
#define GLYPH_HALF       8    /* dar glif genisligi */
#define TILE_TOP_RIGHT   0x40
#define TILE_BOT_LEFT    0x80
#define TILE_BOT_RIGHT   0xC0

extern u32  gTextRowStride;
extern u8  *gTextVramBase;
extern u8  *gGlyphTiles;
extern u32  gHalfLineSpacing;

extern u8   _toupper(u8 ch);
extern s32  GetGlyphWidth(u32 ch);
extern void BlendGlyphAcrossTiles(u16 *dest, s32 subX, const u8 *src, s32 col);

/* 0x08064020 */
void PlaceGlyph(s32 ch, s32 x, s32 y)
{
    s32 width;
    s32 row;
    s32 col;
    s32 subX;
    u8 *glyph;
    u8 *dest;

    if (x > SCREEN_RIGHT)
        return;
    if (y > SCREEN_BOTTOM)
        return;
    if (y < 0)
        return;
    if (x <= -LEFT_LIMIT)
        return;

    if (ch == GLYPH_SUBSTITUTE)
        ch = GLYPH_APOSTROPHE;

    ch = _toupper((u8)ch);
    width = GetGlyphWidth(ch);
    if (ch == GLYPH_SKIP)
        return;

    ch -= GLYPH_FIRST;
    if (ch < 0)
        return;

    if (x & 1)
        x++;
    if (x + width <= 0)
        return;

    row = y >> TILE_SHIFT;
    col = x >> TILE_SHIFT;
    if (row < 0)
        return;
    subX = x - (col << TILE_SHIFT);

    if (gHalfLineSpacing != 0) {
        glyph = gGlyphTiles + (ch << TILE_ORDER);
        dest = gTextVramBase + (col << TILE_ORDER)
             + ((row * gTextRowStride) << TILE_ORDER);
        BlendGlyphAcrossTiles((u16 *)dest, subX, glyph, col);
        return;
    }

    glyph = gGlyphTiles + (ch << GLYPH_ORDER);
    dest = gTextVramBase + (col << TILE_ORDER)
         + ((row * gTextRowStride) << TILE_ORDER);
    BlendGlyphAcrossTiles((u16 *)dest, subX, glyph, col);
    BlendGlyphAcrossTiles((u16 *)(dest + (gTextRowStride << TILE_ORDER)), subX,
                 glyph + TILE_BOT_LEFT, col);

    if (width <= GLYPH_HALF)
        return;

    x += GLYPH_HALF;
    col = x >> TILE_SHIFT;
    subX = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_ORDER)
         + ((row * gTextRowStride) << TILE_ORDER);
    BlendGlyphAcrossTiles((u16 *)dest, subX, glyph + TILE_TOP_RIGHT, col);
    BlendGlyphAcrossTiles((u16 *)(dest + (gTextRowStride << TILE_ORDER)), subX,
                 glyph + TILE_BOT_RIGHT, col);
}
