/* Glifi iki komsu tile'a karistirarak yazma — 0x08063ED8-0x0806401D
 *
 * 8bpp metin katmanina bir glif basiyor. Glif 8 piksel genis, hedef ise
 * 8x8'lik tile'lara bolunmus; piksel ofseti (subX) sifir degilse glif IKI
 * tile'a tasar. Fonksiyon bu yuzden ayni ic dongunun iki kopyasini
 * calistiriyor: once icinde bulunulan tile'in kalan sutunlari
 * (firstPairs), sonra bir sonraki tile'in bas sutunlari (secondPairs).
 * Ikisinin toplami her zaman 4 piksel CIFTI = 8 piksel.
 *
 * Cagiran FUN_08064020: r0 = hedef tile, r1 = subX (x & 7, cift),
 * r2 = glif verisi, r3 = tile sutunu (x >> 3). Sutun araligi [-1, 29];
 * 30 sutun x 8 piksel = 240 piksel ekran genisligi. col == -1, glifin
 * yalnizca SAG yarisinin ekranda oldugu durum: ilk dongu tamamen atlanir,
 * src yine de o kadar ilerletilir. Ikinci tile col+1'de oldugu icin ikinci
 * sinir denetimi nextCol uzerinden yapiliyor.
 *
 * Piksel karistirma: hedef yarim-kelime iki 8bpp pikseli tasir (dusuk bayt
 * = sol piksel). Glif baytinin sifir olmasi SAYDAM demek; o pikselde ekranda
 * duran deger korunuyor. Sifir degilse gFontIndex (secili yazi tipinin
 * karo/palet taban indeksi) ekleniyor ve 16 bite kirpiliyor.
 *
 * ---- OLCULEN BICIM KURALLARI (her biri tek basina fark kapatti) ----
 *
 * 1. ISARETCI YURUYUSU. Ic dongu hedefi satir basina 8 bayt (4 yarim-
 *    kelime), kaynagi 8 bayt ilerletiyor; sekiz satir sonunda ikisi de 62
 *    bayt geri aliniyor, net +2 bayt (bir piksel cifti). Ayri d/s yerelleri
 *    acip `dest++` yazmak agbcc'ye tabani yeniden yukletiyor; ROM tek
 *    yuruyen isaretci kullanip farki cikariyor.
 *
 * 2. AYRI nextCol DEGISKENI. `col++` yazmak col'u fonksiyon basindan
 *    sonuna tek pseudo yapiyor ve girise `mov ip, r3` ekletiyor. ROM'da
 *    parametre r3'te kaliyor, col+1 AYRI bir pseudo olarak sl'ye gidiyor
 *    (kural 27).
 *
 * 3. DONGU SAYACLARI AYRI DEGISKEN. n1/n2 ortak yerel olunca omur iki
 *    dongunun toplami kadar uzuyor, oncelik dusuyor, sayac yigina tasiyor.
 *
 * 4. AZALTMA DONGU TESTINDE (`while (--n1 != 0)`), govdenin basinda degil.
 *    Govde basindaki `n1--` sayaca fazladan bir depth-0 referansi katiyor:
 *    refs 8 -> floor_log2 3 -> oncelik 0.48, row sayacinin 0.412'sinin
 *    ustune cikiyor; sayac dusuk register kapiyor ve reload onu yigina
 *    atiyor. Test icinde azaltinca sayac ip'ye, row r7'ye oturuyor.
 *    TEK BASINA 193 -> 93 bayt fark.
 *
 * 5. IKINCI SAYAC AYRI (n2 = secondPairs). secondPairs'i dogrudan sayac
 *    yapmak ona depth-1 referanslari katip onceligini nextCol'un ustune
 *    cikariyor; o zaman secondPairs ip'yi, nextCol yigini aliyor -- ROM'un
 *    tersi. Ayri n2 ile secondPairs 3 referansa dusup yigina, nextCol
 *    sl'ye gidiyor. 93 -> 48.
 *
 * 6. KAYNAK SIRASI: `n1 = firstPairs;` ONCE, `nextCol = col + 1;` SONRA
 *    (kural 19). Ters sira 89, dogru sira 48 bayt fark.
 *
 * 7. MASKE YERINDE: `cur &= 0xFF`, ayri bir `lo` yereline degil. ROM
 *    `ands r3, r0` ile sonucu cur'un register'inda birakiyor; ayri yerel
 *    hi'yi r6 yerine r3'e itiyor (kural 33'un bu fonksiyondaki hali).
 *
 * 8. SONUC KAYNAK BAYTINA GERI YAZILIYOR (`srcLo = ...`), ayri pixLo/pixHi
 *    yereline degil: ROM'da sonuc srcLo/srcHi ile ayni register'i
 *    paylasiyor. 42 -> 4 bayt.
 *
 * 9. srcLo/srcHi TIPI u16, kirpma ORTULU. Acik `(u16)(srcLo + gFontIndex)`
 *    cast'i toplamanin operand sirasini ceviriyor (`adds r0,r0,r2` yerine
 *    ROM `adds r0,r2,r0`). Son 4 bayt bu satirdaydi. `& 0xFFFF` bicimi ise
 *    tumden bozuyor (243/334).
 *
 * ELENEN YOLLAR: `while (n-- != 0)` (iki pseudo, sayac yigina tasiyor);
 * `for (row = 0; row < 8; row++)` (isaretli geri sayim, `bge` uretiyor,
 * ROM `bne`); lo/hi hesap sirasini cevirmek (agbcc `ldrb` ile tek bayt
 * okumaya doner, 322 bayt); n1'i if/else oncesinde tanimlamak (330/208);
 * gFontIndex'i ayri yerele almak (338/246); guard'i firstPairs uzerinden
 * kurmak (etkisiz, agbcc birlestiriyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/text_f1.c   -> BYTE-MATCHING 326/326
 */

#include "gba_types.h"

#define LAST_TILE_COL   29   /* 240 piksel = 30 tile sutunu */
#define GLYPH_ROWS       8   /* tile yuksekligi */
#define DEST_ROW_STEP    4   /* 8bpp tile satiri = 8 bayt = 4 yarim-kelime */
#define SRC_ROW_SKIP     6   /* satir 8 bayt; ikisi zaten okundu */
#define DEST_ROW_BACK   31   /* 8*4 - 1: sekiz satir sonrasi net +1 yarim-kelime */
#define SRC_ROW_BACK    62   /* 8*8 - 2: sekiz satir sonrasi net +2 bayt */
#define NEXT_TILE       32   /* 64 bayt = bir 8bpp tile (yarim-kelime cinsinden) */
#define TAIL_TO_TILE    28   /* ilk dongu bittiginde sonraki tile'a kalan */

extern u32 gFontIndex;

/* 0x08063ED8 */
void FUN_08063ed8(u16 *dest, s32 subX, const u8 *src, s32 col)
{
    s32 firstPairs;
    s32 secondPairs;
    s32 rem;
    s32 nextCol;
    s32 n1;
    s32 n2;
    s32 row1;
    s32 row2;
    u16 cur;
    u32 hi;
    u16 srcLo;
    u16 srcHi;

    if (col > LAST_TILE_COL)
        return;
    if (col <= -2)
        return;

    /* Sabit 8 iki kez kullaniliyor; ROM da onu tek register'da tutuyor. */
    rem = 8 - subX;
    if (rem <= 7) {
        firstPairs = rem >> 1;
        secondPairs = (8 - rem) >> 1;
    } else {
        firstPairs = 4;
        secondPairs = 0;
    }

    if (col < 0) {
        /* Sol tile ekran disinda: yalnizca isaretcileri ilerlet. */
        dest += NEXT_TILE;
        src += firstPairs * 2;
        nextCol = col + 1;
    } else {
        dest += subX >> 1;
        n1 = firstPairs;
        nextCol = col + 1;
        if (n1 != 0) {
            do {
                for (row1 = GLYPH_ROWS; row1 != 0; row1--) {
                    cur = *dest;
                    hi = (cur & 0xFF00) >> 8;
                    cur &= 0xFF;
                    srcLo = *src++;
                    srcHi = *src++;
                    if (srcLo == 0)
                        srcLo = cur;
                    else
                        srcLo = srcLo + gFontIndex;
                    if (srcHi == 0)
                        srcHi = hi;
                    else
                        srcHi = srcHi + gFontIndex;
                    *dest = (u16)((srcHi << 8) | srcLo);
                    dest += DEST_ROW_STEP;
                    src += SRC_ROW_SKIP;
                }
                dest -= DEST_ROW_BACK;
                src -= SRC_ROW_BACK;
            } while (--n1 != 0);
        }
        dest += TAIL_TO_TILE;
    }

    if (nextCol > LAST_TILE_COL)
        return;
    n2 = secondPairs;
    if (n2 == 0)
        return;

    do {
        for (row2 = GLYPH_ROWS; row2 != 0; row2--) {
            cur = *dest;
            hi = (cur & 0xFF00) >> 8;
            cur &= 0xFF;
            srcLo = *src++;
            srcHi = *src++;
            if (srcLo == 0)
                srcLo = cur;
            else
                srcLo = srcLo + gFontIndex;
            if (srcHi == 0)
                srcHi = hi;
            else
                srcHi = srcHi + gFontIndex;
            *dest = (u16)((srcHi << 8) | srcLo);
            dest += DEST_ROW_STEP;
            src += SRC_ROW_SKIP;
        }
        dest -= DEST_ROW_BACK;
        src -= SRC_ROW_BACK;
    } while (--n2 != 0);
}
