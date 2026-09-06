/* FUN_08023aa0 -- 0x08023AA0-0x08023B21 (130 bayt)
 *
 * Satir satir SAGA kaydirma. Kaynak tamponundan her satir icin bir bayt
 * okuyup `shift` kadar saga kaydiriyor; cikan `n` degeri o satirin kac bayt
 * saga kayacagini soyluyor. Satirin icinde baytlar BITISIK duruyor (adim 1),
 * satirdan satira `stride` bayt atlaniyor. Yurutucu satirin EN SAG ucundan
 * basliyor (dest + length - 1), (length - n) bayti soldan saga cekiyor,
 * sonra solda bosalan n bayti sifirliyor.
 *
 * AILE ICINDEKI YERI -- dorduzun sonuncusu:
 *
 *              satir/sutun ici adim   ilerleme   yon
 *   b3 (c3c)   +stride                +1         yukari  (p artiyor)
 *   b6 (bb8)   +1                     +stride    sola    (p artiyor)
 *   b7 (b24)   -stride                +1         asagi   (p azaliyor)
 *   b8 (aa0)   -1                     +stride    saga    (p AZALIYOR)
 *
 * Dogrudan ayna esi b6. Kontrol akisini yine de kardesten kopyalamadim,
 * ROM'dan okudum (docs/COMPILER.md kural 49 uyarisi) -- ve iyi ki: dongu
 * bicimi b6'dan FARKLI cikti, asagida madde 5.
 *
 * IMZA ROM'DAN OKUNDU, TAHMIN DEGIL:
 *   r0            -> dest   (isaretci, normalize edilmiyor)
 *   r1,r2,r3      -> lsls#24/lsrs#24 cifti var => UCU DE u8
 *   [sp,#28]      -> ayni normalizasyon (0x8023ABE) => u8 (stride)
 *   [sp,#32]      -> normalizasyon yok, sifirla karsilastiriliyor => isaretci
 *   [sp,#36]      -> her turda yeniden okunuyor, asrs ile kullaniliyor => int
 * Prolog `push {r4-r7,lr}` + `mov r7,r9` / `mov r6,r8` / `push {r6,r7}`,
 * yani r8/r9 da kullaniliyor: YEDI callee-saved. Kardes b7'de sekiz vardi,
 * sebebi oradaki `n * stride` carpiminin hoist edilmesiydi; burada eksen
 * degistigi icin o carpim YOK (satir ici adim 1), sekizinci yazmac da yok.
 * Yigin arguman ofsetleri bu yuzden b6 ile ayni (28/32/36), b7 ile degil
 * (32/36/40) -- ofsetleri yanlis kardesten kopyalayan biri burada yanlis
 * parametreleri okur.
 * Donus `pop {r0}; bx r0` ve r0 olu => void (kural 35).
 *
 * ROM'UN DAGITIMI (b6'ninkinden farkli, teshis icin not):
 *   r7 dest   r5 i   r6 src   r4 n   r8 rows   r9 stride   ip length
 * b6'da dest r4, rows ip, length r9, stride r8 idi. Fark kaynaktaki bir
 * tercihten degil: b6'da satir ilerlemesi govde SONUNDA oldugu icin
 * `p + stride` icin ayri bir tasiyici (r7) canli kaliyordu ve dest'i r4'e
 * itiyordu. Burada ilerleme govde ORTASINDA (madde 5), tasiyici yok, dest
 * dogrudan hedef yazmacta kaliyor. Kural 50'nin formulu bunu kendiliginden
 * veriyor; dagitima mudahale gerekmedi.
 *
 * ROM'DAN OLCULEN AYRINTILAR:
 *
 *  1. `p = dest + (length - 1)`: 0x8023AD0-AD4 `mov r0,ip` / `subs r0,#1` /
 *     `adds r2,r7,r0`. b7'deki gibi bir `muls` YOK, cunku satir ici adim 1.
 *  2. `n` u8: 0x8023ADC'deki lsls#24/lsrs#24 cifti. Kaydirma `asrs`
 *     (aritmetik) cunku ldrb'nin sonucu int'e yukseliyor ve `shift` int.
 *  3. Kopyalanacak bayt sayisi 0x8023AE8'de lsls#16/lsrs#16 ile 16 bite
 *     kirpiliyor AMA dongu icindeki azaltma duz `subs r1,#1` -- kirpma
 *     tekrarlanmiyor. Degisken u16 DEGIL; genis bir yerele yazilmis acik
 *     `(u16)` donusumu (kardeslerde de ayni olcum).
 *  4. Dis dongu sayaci isaretsiz: 0x8023ACE `bcs` (kural 31). `int i`
 *     yazimi `bge` verirdi.
 *  5. DONGU BICIMI B6'DAN FARKLI. b6 dondurulmus: tepede `cmp/bcs`, dipte
 *     `cmp/bcc` ile ikinci kez sinanan bir kopya. Burada tepede tek sinama
 *     var ve govde 0x8023B14'te duz `b 0x8023ACC` ile geri donuyor -- dip
 *     kopyasi ve `adds r4,r7,#0` geri yazmasi YOK, tam 4 bayt daha az.
 *     Bunu ureten sey `dest += stride;` ifadesinin YERI: govde ortasinda.
 *     b6'nin basliginda bu yazim "128 bayt (4 eksik)" diye ELENMIS bir yol
 *     olarak duruyor -- orada yanlis, burada DOGRU olan bicim bu. Kardesin
 *     elenenler listesi bu dosya icin gecerli degil.
 *  6. Kopyalama dongusunde ROM once q'yu, sonra p'yi azaltiyor
 *     (0x8023AF8 `subs r3,#1` / 0x8023AFA `subs r2,#1`). Kaynaktaki azaltma
 *     sirasi dogrudan buraya yansiyor; ters yazim kardeste 2 bayt fark
 *     birakmisti.
 *  7. `dest += stride` (0x8023AEC) ile `i++` (0x8023AEE) ic dongulerden
 *     ONCE yayiliyor. i++ kaynakta `for` artiriminda; agbcc onu kendisi
 *     one aliyor.
 *
 * KARDESLERDEN DEVRALINAN, BURADA TEKRAR DENENMEYEN ELEMELER (b6/b7
 * basliklarinda olculmus, ayni duvara toslamayin):
 *  - `u16 remain`: her azaltmadan sonra lsls#16/lsrs#16 ekliyor, ROM'da yok.
 *  - `(u16)` kirpmasini tumden atmak: 4 bayt eksik cikiyor, kirpma kaynakta.
 *  - `int n`: 0x8023ADC'deki daraltma cifti kayboluyor.
 *  - `int stride`: giris normalizasyonu dortlusu ile 14 bayt gidiyor (k.15).
 *  - Sifirlama dongusune AYRI sayac yereli: ayri allocno r0/r1 takasina yol
 *    aciyor. Iki ic dongu TEK sayac degiskenini paylasmali.
 *  - `int i`: `bcs/bcc` yerine `bge/blt`.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b8.c  -> 130/130 eslesti
 */

#include "gba_types.h"

/* 0x08023AA0 */
void FUN_08023aa0(u8 *dest, u8 index, u8 rows, u8 length, u8 stride,
                  u8 *src, int shift)
{
    u8 *p;
    u8 *q;
    u8 n;
    u32 remain;
    u32 i;

    if (src == 0)
        return;
    src += index;
    for (i = 0; i < rows; i++) {
        /* Satirin en sag ucu. Satir ici adim 1 oldugu icin carpim yok. */
        p = dest + (length - 1);
        n = *src >> shift;
        src++;
        q = p - n;

        /* Sagdaki (length - n) bayti n bayt soldan cekip saga kaydir.
         * Kirpma acik: bkz. baslik, madde 3. */
        remain = (u16)(length - n);

        /* Satir ilerlemesi govdenin ORTASINDA -- dongu bicimini bu belirliyor,
         * bkz. baslik madde 5. Kardes b6'da bu yazim yanlisti. */
        dest += stride;

        while (remain != 0) {
            *p = *q;
            q--;      /* ROM once q'yu azaltiyor; sira onemli (madde 6) */
            p--;
            remain--;
        }

        /* Bosalan soldaki n bayti sifirla. Ayni sayac degiskeni: bkz. baslik. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p--;
            remain--;
        }
    }
}
