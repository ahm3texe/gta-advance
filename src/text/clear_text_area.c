/* Metin alani temizleme -- 0x08064590-0x08064613
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
 * PERMUTER SONUCU (2026-09-05): decomp-permuter 24368 iterasyon kostu ve
 * taban skorundan (5) BIR KEZ BILE iyilesmedi.  Tek baytlik fark yazmac
 * dagitimi artefakti; permuter'in yerel kaynak mutasyonlari agbcc'nin
 * dagitim kararini bu yonde oynatmiyor.  Arama uzayinin DISINDA -- daha
 * uzun kosturmak duz zemini daha cok taramak demek.  Kurulum hazir:
 *   python3 tools/make_permuter_dir.py src/text/clear_text_area.c \
 *       ClearTextArea 0x08064590 132
 *
 * ================================================================
 * KURAL 50 ILE TAM TESHIS (2026-09-06) -- ENGEL SAYIYA INDIRILDI
 * ================================================================
 * Onceki teshis ("olu yukleme yazmaci") YANLISTI. tools/dump_alloc.py ile
 * olculen gercek engel TEK BIR SIRALAMA KARARI:
 *
 *     p29 control : refs=5  omur=21  oncelik=0.476   <- sonra islenir
 *     p30 dma     : refs=9  omur=52  oncelik=0.519   <- once isleniyor
 *
 * Cakisma cizgesinde ikisi de {r0,r1,r2} ile catisiyor, yani sirada ONCE
 * gelen find_reg'den r3'u, sonraki r4'u aliyor. ROM control=r3, dma=r4
 * istiyor; yani control'un dma'dan ONCE islenmesi gerekiyor.
 *
 * Iki olu okuma da CIPLAK birakilinca (ROM'un istedigi bicim, iki `ldr r0`
 * de dogru cikiyor) control'un 6. referansi kayboluyor, oncelik 0.545'ten
 * 0.476'ya dusuyor ve sira TERSINE donuyor -> 13 bayt. Su anki 1 baytlik
 * bicim, olu okumayi control'a atayarak o 6. referansi satin aliyor; bedeli
 * yuklemenin r3'e dusmesi. Ikisi ayni anda saglanamiyor CUNKU:
 *
 *   - control'un 6. referansi ANCAK bir komut ureten operand olabilir;
 *     ROM'un 5 komutluk def dizisi (lsl/asr/movs/lsl/orr) control'a en
 *     fazla 3 referans verir, iki store ile toplam 5. Bedava 6. yok.
 *   - control omru 21; kazanmak icin <=19 gerek. Def `asrs`te, son kullanim
 *     blok 3'un 9. komutunda; ikisi de ROM komut sirasina cakili.
 *
 * HEDEF DAGITIMIN ULASILABILIR OLDUGU IKI BAGIMSIZ YOLLA KANITLANDI
 * (ikisi de tek bir artik engelde takiliyor):
 *
 *   1) dma tanimini `ime = REG_IME;` sonrasina almak: omur 52->60, oncelik
 *      0.450 < 0.476, sira duzeliyor. Cikti control=r3 / dma=r4 ve HER IKI
 *      olu okuma `ldr r0` -- 66 komutun 65'i birebir. Tek kusur: `ldr r4,
 *      [pc]` 4 komut erken maddelesiyor (10 bayt). Yerlestirme kuantali:
 *      omur 52 / 56 / 60 / 68; kazanmak icin gereken 57 ARADA KALIYOR,
 *      hicbir C konumu uretmiyor. Olculen dort konum: 13 / 18 / 10 / 17.
 *   2) height'i control olarak kullanip YERINDE hesaplamak
 *      (`height <<= 6; height >>= 1; height |= 0x81000000;`): height refs=9
 *      omur=52 -> oncelik 0.519, dma ile TAM ESITLIK; kural 50'nin
 *      "esitlikte kucuk pseudo once" maddesi geregi p24 once islenip r3'u,
 *      dma r4'u aliyor. ROM dagitiminin AYNISI. Tek kusur kaydirmanin
 *      yerinde olmasi: `lsls r3,r3,#6 / asrs r3,r3,#1` yerine ROM
 *      `lsls r0,r3,#6 / asrs r3,r0,#1` istiyor -- 2 bayt.
 *      Ara degeri ayirinca (`col = height << 6; height = (col>>1)|K;`)
 *      kaydirma DUZELIYOR ama height refs 9->7 dusuyor, floor_log2 bir
 *      basamak iniyor (3->2), oncelik 0.269'a cokuyor, sira bozuluyor: 22.
 *      8 refs de yetmiyor (3*8/52 = 0.4615 < 0.519); esitlik icin TAM 9
 *      gerekiyor ve bedava 8./9. referans yok.
 *
 * Ucuncu yol da kapali: height'i control'dan ayri tutup 8 referansa
 * cikarmak (3*8/28 = 0.857) sirayi cozerdi, ama height ROM'da yalnizca
 * giris kopyasi ve tek `lsls`te geciyor -- 2 referans, 6 bedava referans yok.
 *
 * BU TURDA OLCULEN VE ELENENLER (hepsi taban 1'in ustunde):
 *   - 168 varyantlik kombinasyon taramasi (control def bicimi x blok2 okuma
 *     x CFG bicimi x blok3 okuma hedefi). En iyi 12 sonuc 1'de plato yapti;
 *     hicbiri 0 vermedi. Olu okumanin hedefi olarak x / y / height de
 *     denendi (row/col/dest/fill/ime zaten elenmisti): 1 / 2 / 2.
 *   - dma yerelini TAMAMEN kaldirip 8 erisimi de makroyla yazmak: 13.
 *     CSE tek pseudo uretiyor, refs 9 / omur 52 aynen kaliyor.
 *   - `ctl = &dma->control;` diye AYRI isaretci: 13. agbcc adresi dma+8
 *     olarak katliyor, ayri allocno olusmuyor.
 *   - blok 3'u makroyla, blok 2'yi yerelle yazmak: 13. Yine tek pseudo.
 *   - CFG bicimi TAMAMEN etkisiz: `if (==0)` / `if (!)` / erken return /
 *     goto / bos else -- besi de birebir ayni refs=5,omur=21 ve
 *     refs=9,omur=52 uretiyor (hepsi 13). do-while(0) 15.
 *   - dma omrunu kuyruk okumasiyla uzatmak: refs de 10/11'e cikiyor,
 *     floor_log2 3'te kaliyor, oncelik 0.536/0.569'a YUKSELIYOR: 23 / 44.
 *
 * SONUC: 1 bayt, bu degisken yapisi altinda OLCULMUS optimum. Engel artik
 * "bilinmiyor" degil: control 0.476'ya karsi dma 0.519, ve aradaki farki
 * kapatacak bedava referans/omur kaynagi ROM'un komut dizisinde YOK.
 * Yeni bir fikir denemeden once yukaridaki uc yolun sayilarini kontrol et;
 * onlari tekrarlamak zaman kaybi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/clear_text_area.c
 * Teshis:     python3 tools/dump_alloc.py src/text/clear_text_area.c \
 *                 ClearTextArea --rom --conflicts
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
extern void PlaceGlyph(u8 ch, s32 x, s32 y);

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
        PlaceGlyph(ch, x, y);
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
