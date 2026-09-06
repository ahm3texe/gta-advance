/* Metin satirinin kalanini temizleme — 0x08064180-0x0806430B (396 bayt)
 *
 * NE YAPIYOR
 * Verilen (x, y) pikselinden baslayarak metin katmaninin O SATIRININ
 * SAGINDA kalan her seyi DMA3 ile sifirliyor. Katman 8x8'lik 8bpp
 * tile'lardan olusuyor: bir tile 64 bayt, ekran 30 tile genisliginde
 * (240 piksel). Adres su sekilde kuruluyor:
 *     dest = gTextVramBase + col*64 + (gTextRowStride * row)*64
 * yani ayni kalip src/text/clear_text_area.c'deki ESLESMIS
 * ClearTextArea ile birebir.
 *
 * Is iki asamada yapiliyor:
 *   1. x bir tile'in ortasina dusuyorsa (sub != 0) o tile'in yalnizca
 *      sag kismi temizlenir. 8bpp'de bir tile SATIRI 8 bayt oldugu ve
 *      satirlar bellekte 8'er bayt arayla durdugu icin tek DMA yetmiyor;
 *      sekiz ayri DMA yapiliyor, her biri (8 - sub) bayt yaziyor ve
 *      hedef her turda 8 bayt ilerliyor.
 *   2. Kalan tam tile'lar (col..29) TEK DMA ile temizlenir; onlar
 *      bellekte ardisik oldugu icin (30 - col) * 64 bayt tek blok.
 * Sonra gHalfLineSpacing sifir DEGILSE is biter (yarim satir yuksekligi,
 * 8 piksel). Sifirsa ayni islem bir alt tile satirinda (row + 1)
 * tekrarlanir; tam satir yuksekligi 16 piksel. Bu bayragin anlami
 * clear_text_area.c ve set_text_context.c ile tutarli.
 *
 * Girişte x tek sayiysa bir artiriliyor: 8bpp veri yarim-kelime
 * yazildigi icin baslangicin cift piksel olmasi gerekiyor. Bu sayede
 * sub da her zaman cift, (sub >> 1) * 2 == sub oluyor.
 *
 * DMA kontrol degeri 0x81000000: etkin (bit 31) + kaynak SABIT
 * (bit 24), 16 bit aktarim. Kaynak yigindaki tek bir sifir; yani bu bir
 * "fill" islemi. Sayac yarim-kelime cinsinden, bayt sayisinin yarisi.
 * Yazma SIRASI baglayici: kaynak ve hedef yazilmadan kontrol
 * yazilamaz, cunku aktarim kontrol yazimiyla basliyor.
 *
 * ---------------------------------------------------------------
 * ESLESMIYOR — PARK. Su anki en iyi surum:
 *     bizim 392 bayt / ROM 396 bayt
 *     198 komuttan 132'si BIREBIR, 66'si farkli
 *
 * YAPI DOGRU, KALAN SORUN SAF REGISTER DAGITIMI.
 * Tum bloklar, dallar, sabitler, kaydirma/bolme kaliplari ve dongu
 * bicimi ROM'unkiyle ayni sirada uretiliyor. Ikinci ve dorduncu
 * bloklarin (tam tile temizligi) 19 komutunun TAMAMI birebir; giris
 * kirpmalari, adres kurulumu, dongu on-blogunun bes komutu
 * (`movs r0,#64` / `adds r0,r0,r6` / `mov r9,r0` / `adds r5,#1` /
 * `mov sl,r5`) ve dongu kuyrugu da tam tutuyor.
 *
 * ESKI TESHIS GECERSIZ — DUZELTME (bu oturumda olculdu):
 * Onceki baslik "kok neden x parametresinin ip'ye dusmesi" diyordu.
 * BU ARTIK DOGRU DEGIL: `adds r7, r0, #0` ROM ile birebir esliyor,
 * yani x her iki tarafta da r7'de. O eksende yapacak is yok.
 *
 * GERCEK KOK NEDEN — DONGUDEKI BES DEGERIN YERLESIMI (olculdu):
 * Dongu boyunca su degerler yasiyor: IME taban adresi, sifir sabiti,
 * DMA3 taban adresi, DMA kontrol degeri ve &fill. ROM ile bizim
 * dagitimimiz bunlari neredeyse tam ters yerlestiriyor:
 *
 *     deger          ROM              bizim
 *     -----------    -------------    -------------
 *     DMA kontrol    r3               yigin (sp+12)   <-- kritik
 *     IME tabani     ip (yuksek)      r6
 *     sifir sabiti   r8               ip
 *     &fill          yigin (sp+12)    r8
 *     DMA3 tabani    r5               r3
 *     sayac i        r4               r5
 *
 * Bunun bayt sonucu olculebilir: ROM'un dongu govdesi 13 komut,
 * bizimki 12. IME tabani ROM'da YUKSEK yazmacta oldugu icin her turda
 * IKI kez `mov r0, ip` gerekiyor; bizde r6 low oldugu icin hic
 * gerekmiyor. Dongu basina bir komut x iki dongu = 4 bayt; bu tam
 * olarak 396-392 farkini aciklar. Yani "eksik" 4 bayt kayip kod degil,
 * ROM'un daha kotu dagitimindan gelen fazladan iki `mov`.
 *
 * NEDEN KONTROL DEGERI YIGINA DUSUYOR — OLCULDU (tools/dump_alloc.py):
 *     p67  = birinci yarinin kontrol degeri, refs 5, omur 18,
 *            oncelik = floor_log2(5)*5/18 = 0.556, sira 7  -> SPILL
 *     p128 = ikinci yarinin ayni degeri, ayni oncelik, sira 8 -> r4
 *     p56  = DMA3 tabani, refs 9, omur 48,
 *            oncelik = floor_log2(9)*9/48 = 0.5625, sira 5  -> r3
 * DMA tabani kontrolun HEMEN USTUNDE siralanip r3'u aliyor. Tuhaf
 * olan su: p67'nin cakisma kumesi (--conflicts) r4-r7'yi BOS
 * birakiyor (p24/dest ve p26/col ile cakismiyor), buna ragmen dagitici
 * ona yazmac vermiyor; bir sira sonraki ikizi p128 r4'u aliyor. Yani
 * bu, agbcc global_alloc'un cakisma disi bir maliyet karari; kaynaktan
 * dogrudan surulemiyor. Kural 50 kaldiraci (refs/omur oynatmak)
 * denendi ve asagida elendi.
 *
 * BU OTURUMDA KAZANDIRAN TEK HAMLE (70 -> 66 farkli komut):
 *   `rem` YERELINI KALDIRMAK. `(TILE_WIDTH - sub)` ifadesi dogrudan
 *   dongu icindeki kontrol ifadesine gomuldu. Boylece `8 - sub` bir
 *   dongu degismezi olarak TASINIYOR ve ROM'daki gibi on-blokta,
 *   degismez yuklemelerinden SONRA, kisa omurlu bir gecici olarak
 *   cikiyor -- `movs r0,#8` / `subs r0,r0,r4` ikilisi artik birebir
 *   esliyor. Ayri `rem` yereli ile ayni deger r4'te uzun omurlu bir
 *   allocno oluyor ve tum on-blogu kaydiriyordu (kural 40'in tersi).
 *
 * BU OTURUMDA DENENIP ELENEN YOLLAR (~1200 derleme olculdu;
 * skor = farkli komut sayisi, taban 66):
 *   - ON-BLOK DEYIM SIRASI, 720 permutasyon (pairs/nxt/ncol/rem/dst):
 *     en iyisi asagidaki nxt, ncol, pairs, dst (66). Ikinci en iyi 68,
 *     geri kalani 70-90. Bu eksen TUKETILDI.
 *   - `dst` yerine `dest`i yerinde ilerletmek (ayri isaretci yok):
 *     hicbir siralamada 70'in altina inmedi.
 *   - `dst`i sayacin TURETILMIS dongu degiskeni yapmak, yani dongu
 *     icinde `dest + pairs*2 + (TILE_ROWS-1-i) * TILE_WIDTH` ve dort
 *     benzeri bicim: 75 ve uzeri, belirgin kotu. ROM'un `adds r2,#8`
 *     kuyrugu ancak acik `dst += TILE_WIDTH` ile cikiyor.
 *   - Kontrol degerini kaynakta yerele almak (kural 50 kaldiraci,
 *     refs artirma denemesi): on-blokta yerel 77, dongu icinde yerel
 *     86, tam-tile bloklarinin `control` yereliyle paylastirmak 77.
 *     Satir ici ifade DOGRU olan. Bu eksen KAPALI.
 *   - Dongu govdesi ic sirasi (fill/src/dst/ctl permutasyonlari,
 *     kontrol yazimi SONDA olacak sekilde): hepsi 66. Kontrolu one
 *     alan iki siralama 65 veriyor ama DMA'yi eski kaynakla
 *     baslattigi icin ANLAMCA YANLIS; alinmadi.
 *   - `REG_DMA3.src = (const void *)&fill;` dokumu: fark yok.
 *     `volatile u16 *fillp` yereli: 76, kotu.
 *   - `pairs` yerelini kaldirmak: yine 66 (berabere). Okunabilirlik
 *     icin yerel korundu.
 *
 * ONCEKI OTURUMLARDA ELENENLER (tekrar denenmesin):
 *   - 401 BILDIRIM SIRASI permutasyonu (10 yerel): HICBIRI tek bayt
 *     degistirmedi. docs/COMPILER.md'nin "yigin yerlesimini bildirim
 *     sirasi belirlemez" olcumu burada da dogrulandi. Bu eksen KAPALI.
 *   - `static __inline__ DmaFill(...)` yardimcisi: 149 farkli komut.
 *     Yardimci cagrisi parametre kopyalari uretiyor.
 *   - Ikinci yariya TAMAMEN AYRI yereller (dest2, dst2, sub2, ...):
 *     ayri `fill2` ikinci bir yigin yuvasi acip 142'ye cikardi.
 *   - `volatile DmaChannel *dma` yereli (ClearTextArea bicimi): 90.
 *     ROM DMA tabanini her blokta yeniden yukluyor, yani makro dogru.
 *   - `volatile u16 fill[2]` (kural 20): 104 ve cerceve 412 bayta
 *     cikti. Skaler `volatile u16 fill;` dogru.
 *   - `for (i = 0; i < 8; i++)` (kural 42, artan dongu): 119 farkli.
 *     ROM'un `movs r4,#7 / cmp / bge` bicimi ancak azalan `for` ile
 *     cikiyor. `i = 8; do {} while (--i)` ile ayni sonuc.
 *   - `fill = 0;` ile `REG_IME = 0;` sirasini degistirmek: +11.
 *   - Ikinci yaride `col`/`sub` yeniden hesaplanmamasi: 85 farkli.
 *     ROM ikisini de yeniden hesapliyor, bu dogru.
 *   - `register s32 x asm("r7")` ile x'i sabitlemek 74'e indiriyordu
 *     ama KABUL EDILMEDI (docs/WORKFLOW.md 6 ve review_c_source.py):
 *     byte'lari zorlar, nedenini gizler. Artik gereksiz de: x zaten
 *     kendiliginden r7'ye dusuyor.
 *
 * ONCEKI OTURUMLARIN KAZANDIRAN UC HAMLESI (taban 100 -> 70):
 *   1. `ime`yi her bloga AYRI yerel yapmak (kural 40): tam tile
 *      bloklarinin 19 komutu birden tuttu, `dest` ROM gibi r6'ya indi.
 *   2. `nxt` / `ncol` ara yerelleri: dongu on-blogundaki bes komut
 *      (`movs #64` ... `mov sl,r5`) ROM sirasina oturdu.
 *   3. DMA kontrol sabitini DONGUNUN ICINDE yazmak (kural 21):
 *      dongu-degismezi tasiyicisi onu on-blogun SONUNA koyuyor,
 *      ROM'daki sirayi veren de bu.
 *
 * SIRADAKI ADIM ONERISI: kalan fark tek bir dagitim karari
 * (kontrol degeri mi IME tabani mi low yazmac alacak) etrafinda
 * toplandi ve kaynak duzeyindeki kaldiraclar tuketildi. Bundan
 * sonrasi permuter'in isi; permuter'a "dongu on-blogundaki degismez
 * yuklemelerinin sirasi" ekseni verilmeli.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/text_f3.c
 */

#include "gba_io.h"

#define SCREEN_RIGHT      239   /* 240 piksel genislik */
#define SCREEN_BOTTOM     159   /* 160 piksel yukseklik */
#define TILE_SHIFT          3   /* piksel -> tile sutunu */
#define TILE_WIDTH          8   /* tile basina piksel */
#define TILE_ROWS           8   /* tile basina satir */
#define TILE_BYTES         64   /* 8x8 8bpp bir tile */
#define TILE_BYTE_SHIFT     6   /* << 6 == * TILE_BYTES */
#define TILE_COLUMNS       30   /* 240 / 8 */
#define LAST_TILE_COL      29

/* DMA3 kontrolu: etkin (bit 31) + kaynak sabit (bit 24), 16 bit. */
#define DMA_FILL_HALFWORDS 0x81000000

extern u8  *gTextVramBase;     /* 0x02036310 */
extern u32  gTextRowStride;    /* 0x0203630C -- tile cinsinden satir adimi */
extern u32  gHalfLineSpacing;  /* 0x0203631C -- sifir degilse tek tile satiri */

/* 0x08064180 */
void FUN_08064180(s32 x, s32 y)
{
    volatile u16 fill;   /* DMA'nin sabit kaynagi; yiginda sp+0 */
    u8 *dest;            /* temizlenecek ilk tile'in bayt adresi */
    u8 *dst;             /* yarim tile dongusunun yurutucusu */
    u8 *nxt;             /* yarim tile bitince gecilecek tam tile */
    s32 row;
    s32 col;
    s32 ncol;            /* yarim tile bitince gecilecek sutun */
    s32 sub;             /* x'in tile ici piksel ofseti */
    s32 pairs;           /* atlanacak piksel cifti sayisi */
    s32 i;
    s32 control;

    /* 8bpp veri yarim-kelime yazildigi icin baslangic cift olmali. */
    if ((x & 1) != 0)
        x++;
    if ((u32)x > SCREEN_RIGHT)
        return;
    if ((u32)y > SCREEN_BOTTOM)
        return;

    row = y >> TILE_SHIFT;
    col = x >> TILE_SHIFT;
    sub = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_BYTE_SHIFT)
         + (gTextRowStride * row << TILE_BYTE_SHIFT);

    /* Ust tile satiri, birinci asama: yarim tile. Tile satirlari
     * bellekte 8'er bayt arayla durdugu icin tek DMA yetmiyor,
     * sekiz ayri aktarim yapiliyor. */
    if (sub != 0) {
        u16 ime;

        nxt = dest + TILE_BYTES;
        ncol = col + 1;
        pairs = sub >> 1;
        dst = dest + pairs * 2;
        for (i = TILE_ROWS - 1; i >= 0; i--) {
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = &fill;
            REG_DMA3.dst = dst;
            /* Kural 21: sabit dongunun ICINDE yazilir; degismez
             * tasiyicisi onu ROM'daki gibi on-blogun sonuna koyuyor.
             * (TILE_WIDTH - sub) de burada duruyor -- ayri bir `rem`
             * yereli yapmak dagitimi bozuyor, baslikdaki nota bak. */
            REG_DMA3.control = ((TILE_WIDTH - sub) / 2) | DMA_FILL_HALFWORDS;
            REG_DMA3.control;
            REG_IME = ime;
            dst += TILE_WIDTH;   /* bir tile satiri = 8 bayt */
        }
        dest = nxt;
        col = ncol;
    }

    /* Ikinci asama: kalan tam tile'lar bellekte ardisik, tek blok. */
    if (col <= LAST_TILE_COL) {
        u16 ime;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        REG_DMA3.src = &fill;
        REG_DMA3.dst = dest;
        control = (((TILE_COLUMNS - col) << TILE_BYTE_SHIFT) >> 1)
                | DMA_FILL_HALFWORDS;
        REG_DMA3.control = control;
        REG_DMA3.control;
        REG_IME = ime;
    }

    /* Yarim satir yuksekligi: alt tile satiri temizlenmez. */
    if (gHalfLineSpacing != 0)
        return;

    /* col ve sub ROM'da yeniden hesaplaniyor; onbelleklemek 85 komut
     * fark birakiyor (baslikdaki elenenler listesine bak). */
    col = x >> TILE_SHIFT;
    sub = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_BYTE_SHIFT)
         + (gTextRowStride * (row + 1) << TILE_BYTE_SHIFT);

    /* Alt tile satiri: ustteki iki asamanin birebir ayni kopyasi. */
    if (sub != 0) {
        u16 ime;

        nxt = dest + TILE_BYTES;
        ncol = col + 1;
        pairs = sub >> 1;
        dst = dest + pairs * 2;
        for (i = TILE_ROWS - 1; i >= 0; i--) {
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = &fill;
            REG_DMA3.dst = dst;
            REG_DMA3.control = ((TILE_WIDTH - sub) / 2) | DMA_FILL_HALFWORDS;
            REG_DMA3.control;
            REG_IME = ime;
            dst += TILE_WIDTH;
        }
        dest = nxt;
        col = ncol;
    }

    if (col <= LAST_TILE_COL) {
        u16 ime;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        REG_DMA3.src = &fill;
        REG_DMA3.dst = dest;
        control = (((TILE_COLUMNS - col) << TILE_BYTE_SHIFT) >> 1)
                | DMA_FILL_HALFWORDS;
        REG_DMA3.control = control;
        REG_DMA3.control;
        REG_IME = ime;
    }
}
