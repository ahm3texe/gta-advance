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
 *
 * ---------------------------------------------------------------
 * ESLESMIYOR — PARK. Su anki en iyi surum:
 *     bizim 392 bayt / ROM 396 bayt
 *     198 komuttan 128'i BIREBIR, 70'i farkli
 *     bayt farki 187/396, ilk fark 0x0806419C
 *
 * YAPI DOGRU, KALAN SORUN SAF REGISTER DAGITIMI.
 * Tum bloklar, dallar, sabitler, kaydirma/bolme kaliplari ve dongu
 * bicimi ROM'unkiyle ayni sirada uretiliyor. Ornegin ikinci ve
 * dorduncu bloklarin (tam tile temizligi) 19 komutunun TAMAMI birebir:
 *     ldrh r3,[r4,#0] / movs r1,#0 / strh r1,[r4,#0] / mov r0,sp /
 *     strh r1,[r0,#0] / ... / ldr r0,[r2,#8] / strh r3,[r4,#0]
 * Ayni sekilde dongu on-bloku (`movs r0,#64` / `adds r0,r0,r6` /
 * `mov r9,r0` / `adds r5,#1` / `mov sl,r5`) ve dongu kuyrugu
 * (`adds r2,#8` / `subs r4,#1` / `cmp r4,#0` / `bge`) tam tutuyor.
 *
 * TEK KOK NEDEN: `x` parametresi. ROM onu r7'de tutuyor
 * (`adds r7, r0, #0`), agbcc bizde ip'ye koyuyor (`mov ip, r0`).
 * ip yuksek register oldugu icin Thumb'da `ands`, `cmp`, `adds #imm`
 * ile kullanilamiyor; agbcc her kullanim icin `mov rN, ip` ekliyor.
 * Bu tek kayma zincirleme olarak dongudeki dort degerin de
 * (IME tabani, sifir sabiti, DMA tabani, &fill) ROM'dakinden farkli
 * register'lara dusmesine yol aciyor.
 *
 * NEDEN ip'ye dusuyor — OLCULDU (tools/dump_alloc.py, `-dg` dokumu):
 *     x = pseudo 22, refs = 8, live_length = 94
 *     oncelik = floor_log2(8) * 8 / 94 = 0.255
 *     dagitim sirasinda 27 pseudo icinde 16. sirada
 * O sirada r5/r6/r7 daha yuksek oncelikli pseudolarla (ctl, IME
 * tabani, sifir sabiti) dolu; x r0-r4 ile CAKISIYOR (dokumdeki
 * "22 conflicts: ... 0 1 2 3 4 13"), geriye yalnizca ip kaliyor.
 * ROM'da ayni yerde r7 bosta kalmis, yani ROM'un dagiticisi x'i daha
 * ERKEN dagitmis; bu da x'in orada ya daha cok referansi ya da daha
 * kisa omru oldugu anlamina geliyor.
 *
 * MEKANIZMA DOGRULANDI: x'in omru kisaltilinca (ikinci yaridaki
 * `sub = x - (col << 3)` kaldirilarak) x ANINDA dusuk register'a
 * (r3) tasindi. Yani teshis dogru; ama o degisiklik ROM'daki
 * `lsls r0,r5,#3 / subs r4,r7,r0` ikilisini yok ettigi icin toplam
 * fark kotulesiyor (138 -> 170). Gerekli olan sey x'i r3'e degil
 * r7'ye dusurecek ARA bir oncelik; formule gore bu, omur 94'te
 * kalirken refs'in 8'den ~13'e cikmasini gerektiriyor ve kaynakta
 * x'e dogal olarak bes referans daha eklemenin yolu bulunamadi.
 *
 * DENENIP ELENEN YOLLAR (hepsi olculdu; skor = farkli komut sayisi,
 * taban surum 70):
 *   - 401 BILDIRIM SIRASI permutasyonu (10 yerel): HICBIRI tek bayt
 *     degistirmedi. docs/COMPILER.md'nin "yigin yerlesimini bildirim
 *     sirasi belirlemez" olcumu burada da dogrulandi. Bu eksen KAPALI.
 *   - `static __inline__ DmaFill(...)` yardimcisi (iki yarinin ayni
 *     kod olmasi bunu dusunduruyor): 149 farkli komut, cok daha kotu.
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
 *   - `pairs` yerelini kaldirmak / `dst`i ifade icine gommek: etkisiz.
 *   - Blok icindeki bes deyimin (pairs, nxt, ncol, ctl, dst) 60
 *     permutasyonu: en iyisi asagida kullanilan sira.
 *   - Yerellerin blok kapsami x fonksiyon kapsami icin 2^9 = 512
 *     kombinasyon + 4 deyim sirasi = 2048 derleme tarandi; en iyi
 *     kombinasyon asagidaki (ime/imeb blok kapsaminda, dst/pairs/i/
 *     nxt/ncol fonksiyon kapsaminda).
 *   - Ikinci yaride `col`/`sub` yeniden hesaplanmamasi: 85 farkli.
 *     ROM ikisini de yeniden hesapliyor, bu dogru.
 *   - `register s32 x asm("r7")` ile x'i sabitlemek 74'e indiriyor
 *     ama KABUL EDILMEDI (docs/WORKFLOW.md 6 ve review_c_source.py):
 *     byte'lari zorlar, nedenini gizler. Yalnizca teshis icin
 *     kullanildi ve dosyaya girmedi.
 *
 * KAZANDIRAN UC HAMLE (taban 100 -> 70 farkli komut):
 *   1. `ime`yi her bloga AYRI yerel yapmak (kural 40): tam tile
 *      bloklarinin 19 komutu birden tuttu, `dest` ROM gibi r6'ya indi.
 *   2. `nxt` / `ncol` ara yerelleri: dongu on-blogundaki bes komut
 *      (`movs #64` ... `mov sl,r5`) ROM sirasina oturdu.
 *   3. DMA kontrol sabitini DONGUNUN ICINDE yazmak (kural 21):
 *      dongu-degismezi tasiyicisi onu on-blogun SONUNA koyuyor,
 *      ROM'daki sirayi veren de bu.
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
    u8 *dest;
    u8 *dst;
    u8 *nxt;
    s32 row;
    s32 col;
    s32 ncol;
    s32 sub;
    s32 pairs;
    s32 rem;
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

    /* Ust tile satiri: once yarim tile, sonra kalan tam tile'lar. */
    if (sub != 0) {
        u16 ime;

        pairs = sub >> 1;                /* atlanacak piksel cifti */
        nxt = dest + TILE_BYTES;
        ncol = col + 1;
        rem = TILE_WIDTH - sub;          /* temizlenecek bayt sayisi */
        dst = dest + pairs * 2;
        for (i = TILE_ROWS - 1; i >= 0; i--) {
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = &fill;
            REG_DMA3.dst = dst;
            /* Kural 21: sabit dongunun ICINDE yazilir. */
            REG_DMA3.control = (rem / 2) | DMA_FILL_HALFWORDS;
            REG_DMA3.control;
            REG_IME = ime;
            dst += TILE_WIDTH;           /* bir tile satiri = 8 bayt */
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

    /* Yarim satir yuksekligi: alt tile satiri temizlenmez. */
    if (gHalfLineSpacing != 0)
        return;

    col = x >> TILE_SHIFT;
    sub = x - (col << TILE_SHIFT);
    dest = gTextVramBase + (col << TILE_BYTE_SHIFT)
         + (gTextRowStride * (row + 1) << TILE_BYTE_SHIFT);

    /* Alt tile satiri: ustteki islemin birebir ayni kopyasi. */
    if (sub != 0) {
        u16 ime;

        pairs = sub >> 1;
        nxt = dest + TILE_BYTES;
        ncol = col + 1;
        rem = TILE_WIDTH - sub;
        dst = dest + pairs * 2;
        for (i = TILE_ROWS - 1; i >= 0; i--) {
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = &fill;
            REG_DMA3.dst = dst;
            REG_DMA3.control = (rem / 2) | DMA_FILL_HALFWORDS;
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
