/* Metin alani temizleme — 0x08064590-0x08064613
 *
 * Verilen hucreden baslayarak metin katmanini DMA3 ile sifirlar. Ikinci
 * blok yalnizca gHalfLineSpacing sifirken calisir (tam satir yuksekligi).
 *
 * HENUZ ESLESMIYOR: 66 komutun 65'i birebir tutuyor, TEK BAYT fark var
 * (0x080645F2). Fark, DMA kontrol yazmacindan yapilan OLU OKUMANIN hedef
 * register'i:
 *     ROM  : ldr r0, [r4, #8]
 *     bizim: ldr r3, [r4, #8]
 * Deger atiliyor, yani semantik fark yok -- saf register dagitimi.
 * NEDENI OLCULDU (docs/COMPILER.md, register dagitimi): iki okuma da ciplak
 * birakilinca control r4'e, dma r3'e dusuyor -- ROM'un tersi, 13 bayt. Ciplak
 * okuma dogru hedefi (r0) veriyor ama control'un 6. referansini goturuyor ve
 * dagitim siralamasi ters donuyor. Ikisi ayni anda saglanamiyor.
 *
 * Denenenler (hepsi daha kotu): ayri discard degiskeni 13, ciplak deyim 13,
 * ikisini de control'a atamak 24, olu okumayi row'a 13 / col'a 20 /
 * dest'e 13 / fill'e 57 / ime'ye 56, ilk bloktaki okumayi degistirmek 63-103.
 * Yedi yerel degiskenin 5040 bildirim permutasyonu tarandi: hicbiri 1'in
 * altina inmedi. Ek olarak elenenler:
 *   - height'i control olarak yeniden kullanmak (ROM'daki `add r3,r2,#0`
 *     ipucundan): 15
 *   - dma tanimini one almak, dagitici omrunu uzatip onceligini dusursun diye:
 *     tanim noktasina gore 13 / 17 / 56 / 64 -- sabit yuklemesi yazildigi
 *     yerde maddelestigi icin komut sirasi kayiyor, kazanc yok
 *   - DMA kurulumunu iki kez cagrilan inline yardimciya almak: 55-59
 * Bu bicim olculmus yerel optimum.
 *
 * PERMUTER DE KIRAMADI (decomp-permuter-agbcc, tools/setup_permuter.sh):
 * 13.917 yinelemede taban skor 205'in ALTINA inilemedi; yalnizca esit
 * skorlu farkli bicimler uretildi. Toplam kanit: 5040 bildirim
 * permutasyonu + 18 hedefli elle deneme + 13.917 rastgele deneme.
 * Artik "henuz bulamadik" degil, OLCULMUS bir duvar. Kaynak duzeyinde
 * yeniden duzenlemeyle ulasilabilir gorunmuyor; cozum muhtemelen baska
 * bir yerde (orn. fonksiyonun ait oldugu gercek ceviri biriminin
 * bilinmesi, ya da henuz olcmedigimiz bir agbcc davranisi).
 *
 * TERS YON DE DENENDI VE ELENDI. SetBg1Enable'i cozen "yerelleri KALDIR"
 * yontemi burada dort varyantla sinandi, dordu de kotulesti:
 *     control yerelini kaldirmak                 13
 *     row/col kaldirip dest'i tek ifade         116
 *     ikinci blokta da ciplak okuma              13
 *     dest yerelini kaldirmak                   111
 * Taban 1; hicbiri yaklasamadi bile. Boylece bu fonksiyon HER IKI YONDEN
 * kapali: yerel ekleme (18 deneme), yerel kaldirma (4 deneme), rastgele
 * arama (13.917 yineleme) ve 5040 bildirim permutasyonu.
 *
 * ENGEL DAHA KESIN TANIMLANDI (kural 35-38 turevleri denendi): sorun "olu
 * okumanin register'i" DEGIL. Ciplak okuma + control'u UC DEYIMDE hesaplamak
 * 0x62'yi TAM OLARAK duzeltiyor (olu okuma r0'a dusuyor) ama farki 0x38/0x3a'ya
 * tasiyor:
 *     ROM  : lsl r0, r3, #6  /  asr r3, r0, #1   <- ara deger r0'dan geciyor
 *     bizim: lsl r3, r3, #6  /  asr r3, r3, #1   <- yerinde
 * Yani gercek engel: agbcc kaydirmayi r0 uzerinden gecirirken control'un
 * dagitimini ayni anda koruyamiyor. Iki bagimsiz yol da bu ayni iki bayta
 * cikiyor (uc deyim = 2, dort deyim yerinde = 2).
 *
 * Bu turda ayrica elenenler: ikinci bloga ayri taban kopyasi (kural 37) 13 --
 * `dma2 = dma` saf kopya oldugu icin agbcc birlestiriyor; ayri `shifted` ara
 * yereli 13 (yeni bildirim referans dengesini bozuyor); olu yerelleri (row 58,
 * col 20, height 13) ara deger yapmak; sabiti one alip |= ile birlestirmek 60.
 *
 * NOT: bir ajan `register volatile DmaChannel *dma asm("r4");` ile 0 bayta
 * ulasti. Bu KABUL EDILMEDI (docs/WORKFLOW.md 6): acik register baglamasi
 * byte'lari tutturur ama nedenini gizler ve her register uyusmazligini
 * "cozebilecek" bir cekictir. Yine de bir bilgi veriyor: ROM'un r4 secimi
 * ulasilabilir, yani sorun dogal C'de o secimi tetikleyecek bicimi bulmak.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/clear_text_area.c
 */

#include "gba_io.h"

#define GLYPH_FIRST      32
#define GLYPH_SUBSTITUTE 146
#define GLYPH_APOSTROPHE 39
#define COLOUR_ESCAPE    64
#define SCREEN_RIGHT     239
#define SCREEN_BOTTOM    159
#define LINE_HEIGHT      16
#define LINE_HEIGHT_HALF 8
#define TILE_SHIFT       3

extern u32 gTextRowStride;
extern u8 *gTextVramBase;
extern u8 *gGlyphWidths;
extern u32 gFontIndex;
extern u32 gHalfLineSpacing;

extern u8   _toupper(u8 ch);
extern void FUN_08064020(u8 ch, s32 x, s32 y);

s32 GetGlyphWidth(u32 ch);
s32 GetTextWidth(const u8 *text);

#define IS_COLOUR_DIGIT(c) ((u8)((c) - 56) <= 1 || (c) == 48)

static __inline__ s32 GlyphAdvance(u32 ch)
{
    s32 index;
    u32 width;
    s32 advance;

    index = ch;
    if (index == GLYPH_SUBSTITUTE)
        index = GLYPH_APOSTROPHE;

    index = _toupper((u8)index);
    index -= GLYPH_FIRST;
    if (index < 0)
        return 0;

    width = gGlyphWidths[index];
    advance = width + 2;
    if (advance & 1)
        advance = width + 3;

    return advance;
}

static __inline__ s32 DrawTextAt(const u8 *text, s32 x, s32 y)
{
    s32 left;
    u8 ch;

    left = x;

    while ((ch = *text++) != 0) {
        if (ch == COLOUR_ESCAPE && IS_COLOUR_DIGIT(*text)) {
            text++;
            continue;
        }

        if (ch == 10 || ch == 13) {
            x = left;
            if (gHalfLineSpacing == 0)
                y += LINE_HEIGHT;
            else
                y += LINE_HEIGHT_HALF;
            continue;
        }

        ch = _toupper(ch);
        FUN_08064020(ch, x, y);
        x += GetGlyphWidth(ch);
        if (x > SCREEN_RIGHT)
            break;
    }

    return x;
}

/* 0x08064590 */
void ClearTextArea(s32 x, s32 y, s32 height)
{
    volatile u16 fill;
    u16 ime;
    u8 *dest;
    s32 row;
    s32 col;
    s32 control;
    volatile DmaChannel *dma;

    if ((u32)x > SCREEN_RIGHT)
        return;
    if ((u32)y > SCREEN_BOTTOM)
        return;

    row = y >> TILE_SHIFT;
    col = x >> TILE_SHIFT;
    dest = gTextVramBase + (col << 6) + (row * gTextRowStride << 6);

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma = (volatile DmaChannel *)REG_DMA3_ADDR;
    dma->src = &fill;
    dma->dst = dest;
    control = ((height << 6) >> 1) | 0x81000000;
    dma->control = control;
    dma->control;
    REG_IME = ime;

    if (gHalfLineSpacing == 0) {
        dest += gTextRowStride << 6;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        dma->src = &fill;
        dma->dst = dest;
        dma->control = control;
        control = dma->control;
        REG_IME = ime;
    }
}
