/* Metin katmani kurulumu ve temizligi -- 0x08064614-0x08064697
 *
 * Metin cizim baglamini kurar, hedef VRAM bolgesini DMA3 ile sifirlar ve
 * yazi paletini palet RAM'ine kopyalar. Dallanma yok; tek dogrusal akis.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 *
 * 1. HEDEF ADRES. `lsls r5,r5,#5` + (0xC0 << 19) yani VRAM_BASE + tile*32.
 *    32 bayt = bir 4bpp karo, yani birinci parametre KARO INDEKSI.
 *    0x06000000 havuzdan okunmuyor, `movs #192 / lsls #19` ile
 *    hesaplaniyor -- docs/COMPILER.md'nin "agbcc'nin kaydirmayla
 *    uretebildigi adresler sabit cast olarak yazilmis" notu; VRAM_BASE
 *    makrosu bunu aynen veriyor, extern sembol GEREKMIYOR (kural 1 burada
 *    gecerli degil, adres ROM/VRAM sabiti).
 *
 * 2. SetTextContext ARGUMANLARI. r0=dest, r1=28, r2=0x0884DE40,
 *    r3=0x088516C0, sp+0=(u8)(palette+bias), sp+4=1. Imza kardes dosyadan
 *    (src/text/set_text_context.c) aynen alindi.
 *
 * 3. TUTARLILIK KONTROLU -- yorum uydurma degil, sayilar birbirini
 *    dogruluyor: stride 28 karo x 64 bayt = 1792 bayt = 0x380 yarim-kelime,
 *    ve temizleme DMA'sinin sayaci TAM OLARAK 0x380. Ikinci DMA 0x10
 *    yarim-kelime = 16 renk = tek 4bpp palet; hedef 0x05000000 + palette*2
 *    oldugu icin ikinci parametre PALET GIRIS INDEKSI (renk cinsinden).
 *
 * 4. IKI ARGUMANIN TOPLAMI. `adds r2,r6,r2` + `lsls/lsrs #24`: fontIndex
 *    parametresi palette+bias'in u8'e kirpilmisi. Kirpma cagiranda
 *    goruluyor cunku prototipteki parametre `u8` (asagiya bak).
 *
 * ---- OLCULEN BICIM KURALLARI (her biri ayri ayri sinandi) ----
 *
 * Bu fonksiyon ILK DENEMEDE eslesti. Asagidaki sayilar eslesmeden SONRA,
 * hangi yazim tercihinin gercekten tasiyici oldugunu belgelemek icin
 * olculdu -- yani "denedim tutmadi" degil, "kaldirinca sunu bozuyor".
 *
 *   fontIndex prototipte `u8` (`u32` yapmak)              -> 107 bayt fark
 *       `u32` olunca cagirandaki `lsls #24 / lsrs #24` cifti tumden
 *       kayboluyor ve fonksiyon 128 bayta iniyor (kural 15'in cagiran
 *       tarafi). Kardes tanimin kendi ic kirpmasi bunu KARSILAMIYOR.
 *   `fill` yigin tamponu `volatile`                       ->  61 bayt fark
 *       Kural 3. Kaldirilinca agbcc adres almayi sabit yuklemesiyle
 *       yeniden siraliyor.
 *   `fill = 0;` src atamasindan ONCE                      ->   8 bayt fark
 *       ROM sirasi: `strh r3,[r0]` sonra `str r0,[r1,#0]`. Kural 19.
 *   Olu `REG_DMA3.control;` okumasi CIPLAK                ->  85 bayt fark
 *       Degeri `ime`ye atamak dagitimi bozuyor ve fonksiyonu 136 bayta
 *       cikariyor. clear_text_area.c'de TERSI gerekiyordu (orada okuma bir
 *       yerele atanmali) -- ayni ailede ayni deyim iki farkli bicim
 *       istiyor, ezberlenmez, iki yonu de olcun.
 *   `REG_DMA3` makrosu, ayri `dma` yereli DEGIL           ->  17 bayt fark
 *       menu_graphics.c ile ayni tercih; `volatile DmaChannel *dma`
 *       yereli acmak havuz yuklemesini one alip sirayi kaydiriyor.
 *
 * FARK YARATMAYAN (hepsi 0 -- bu satirlarda serbestsiniz):
 *   - `(u8)(palette + bias)` acik cast'i: prototip zaten donusturuyor.
 *   - Parametreleri `s32` yapmak: hepsi yalnizca kaydirma/toplama ile
 *     kullanildigi icin isaretlilik komut secmiyor.
 *   - `dest` yerelini kaldirip ifadeyi iki kez yazmak: CSE birlestiriyor.
 *   - Ikinci blok icin ayri `ime2` yereli: omur ortusmedigi icin ayni
 *     yazmaca dusuyor.
 *   - Palet hedefini `(u16 *)PALETTE_BASE + palette` diye yazmak.
 *
 * ADLANDIRMA: uc parametrenin anlami ROM'daki kullanimdan cikarildi
 * (karo indeksi / palet indeksi / font sapmasi). GLYPH_TILES ve
 * GLYPH_WIDTHS adlari SetTextContext'in 3. ve 4. parametrelerinin kardes
 * dosyada aldigi adlardan geliyor; oradaki `gGlyphTiles` adi da GECICI
 * olarak isaretli. Fonksiyon adi InitTextTilesAndPalette olarak birakildi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/text_f4.c  -> BYTE-MATCHING 132/132
 */

#include "gba_io.h"

#define TEXT_TILE_COUNT  28              /* 28 karo x 64 bayt = 0x380 yarim-kelime */
#define GLYPH_TILES      ((u8 *)0x0884DE40)
#define GLYPH_WIDTHS     ((u8 *)0x088516C0)
#define TEXT_PALETTE     ((const void *)0x0884DC40)
#define PALETTE_BASE     0x05000000
#define DMA_CLEAR_TEXT   0x81000380      /* sabit kaynak, 16 bit, 0x380 adet */
#define DMA_COPY_PALETTE 0x80000010      /* 16 renk */

extern void SetTextContext(u8 *vram, u32 stride, u8 *tiles, u8 *widths,
                           u8 fontIndex, u32 halfSpacing);

/* 0x08064614 */
void InitTextTilesAndPalette(u32 tile, u32 palette, u32 bias)
{
    volatile u16 fill;
    u16 ime;
    u8 *dest;

    dest = (u8 *)(VRAM_BASE + (tile << 5));

    SetTextContext(dest, TEXT_TILE_COUNT, GLYPH_TILES, GLYPH_WIDTHS,
                   palette + bias, 1);

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = dest;
    REG_DMA3.control = DMA_CLEAR_TEXT;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = TEXT_PALETTE;
    REG_DMA3.dst = (void *)(PALETTE_BASE + (palette << 1));
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;
}
