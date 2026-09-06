/* FUN_08023bb8 -- 0x08023BB8-0x08023C3B (132 bayt)
 *
 * Satir satir kaydirma. Kaynak tamponundan her satir icin bir bayt okuyup
 * `shift` kadar saga kaydiriyor; cikan `n` degeri o satirin kac bayt sola
 * kayacagini soyluyor. Satirin icinde baytlar BITISIK duruyor (adim 1),
 * satirdan satira `stride` bayt atlaniyor. Once (length - n) bayt sagdan
 * sola cekiliyor, sonra sagda kalan n bayt sifirlaniyor.
 *
 * KARDESIYLE ILISKISI -- bu fonksiyon FUN_08023c3c'nin (entries_b3.c)
 * EKSEN DEGISTIRILMIS ikizi. Ayni imza, ayni kontrol akisi; tek fark iki
 * adimin yer degistirmesi:
 *
 *              satir ici adim      satirdan satira adim
 *   b3 (c3c)   stride              1        (sutun sutun)
 *   b6 (bb8)   1                   stride   (satir satir)
 *
 * Bu yuzden b3'te gereken `n * stride` carpimi burada YOK: ROM 0x8023BF6'da
 * dogrudan `adds r3, r2, r4` ile q = p + n hesapliyor. Kontrol akisini yine
 * de kardesten kopyalamadim, ROM'dan okudum (docs/COMPILER.md uyarisi);
 * bu ikisinde tesadufen ayni cikti.
 *
 * IMZA ROM'DAN OKUNDU, TAHMIN DEGIL:
 *   r0            -> dest   (isaretci, normalize edilmiyor)
 *   r1,r2,r3      -> lsls#24/lsrs#24 cifti var => UCU DE u8
 *   [sp,#28]      -> ayni normalizasyon (0x8023BD6) => u8 (stride)
 *   [sp,#32]      -> normalizasyon yok, sifirla karsilastiriliyor => isaretci
 *   [sp,#36]      -> her turda yeniden okunuyor, asrs ile kullaniliyor => int
 * Prolog `push {r4-r7,lr}` + `mov r7,r9`/`mov r6,r8`/`push {r6,r7}`, yani
 * r8/r9 da kullaniliyor. Donus `pop {r0}; bx r0` ve r0 olu => void (kural 35).
 *
 * ROM'UN DAGITIMI (b3'unkinden farkli, teshis icin not):
 *   r4 dest (sonra n)  r5 i  r6 src  r7 next  r8 stride  r9 length  ip rows
 * b3'te stride iki ic dongude de kullanildigi icin r5'e, satir sayisi r8'e
 * dusuyordu. Burada stride'in tek referansi var (p + stride), o yuzden r8'de
 * kaliyor ve dis dongu siniri ip'e cikiyor. Yani dagitim farki kaynaktaki
 * bir tercihten degil, eksen degisiminin referans sayimlarini degistirmesinden
 * geliyor -- kural 50'nin formulu bunu kendiliginden veriyor, mudahale gerekmedi.
 *
 * ROM'DAN OLCULEN AYRINTILAR:
 *
 *  1. `n` u8: 0x8023BF0'daki lsls#24/lsrs#24 cifti. Kaydirma `asrs`
 *     (aritmetik) cunku ldrb'nin sonucu int'e yukseliyor ve `shift` int.
 *  2. Kopyalanacak bayt sayisi 0x8023BFC'de lsls#16/lsrs#16 ile 16 bite
 *     kirpiliyor AMA dongu icindeki azaltma duz `subs r1,#1` -- kirpma
 *     tekrarlanmiyor. Yani degisken u16 DEGIL; genis bir yerele yazilmis
 *     acik `(u16)` donusumu.
 *  3. Dis dongu sayaci isaretsiz: 0x8023BE6 `bcs` ve 0x8023C2E `bcc`
 *     (kural 31).
 *  4. Kopyalama dongusunde ROM once q'yu, sonra p'yi artiriyor
 *     (0x8023C0E `adds r3,#1` / 0x8023C10 `adds r2,#1`). Kaynaktaki artirim
 *     sirasi dogrudan buraya yansiyor -- kardeste sira tersti (p once).
 *  5. Satir ilerlemesi govdenin SONUNDA. ROM p+stride'i erken hesaplayip
 *     r7'de bekletiyor (0x8023C02) ve dongu dibinde `adds r4,r7,#0` ile geri
 *     yaziyor; bu, artirimi sona koyunca kendiliginden cikiyor.
 *
 * DENENIP ELENEN YAZIMLAR (en degerli kisim, ayni duvara toslamayin):
 *
 *  - Kopyalama dongusunde `p++; q++;` sirasi (ROM'un tersi): 2/132 fark.
 *    Tek dugme artirimlarin KAYNAK SIRASI; baska hicbir sey degismiyor.
 *  - `int i` (isaretli dis sayac): 2/132 fark, iki dal `bcs/bcc` yerine
 *    `bge/blt` oluyor. Kural 31'in dogrudan dogrulanmasi.
 *  - Satir ilerlemesinin yeri, dort varyant olculdu (hepsi 132 hedefine
 *    karsi):
 *        `dest = p + stride;` govde ortasinda -> 128 bayt (4 eksik)
 *        `for (i = ...; i++, dest += stride)` -> 132, fark  6
 *        ayri `next = p + stride; ... dest = next` -> 132, fark 11
 *        `dest += stride;` govde SONUNDA      -> 132, fark  0  (dogru bicim)
 *    Ortadaki bicimde agbcc `dest`i dogrudan hedef yazmaca dagitip dongu
 *    dibindeki kopyayi eliyor; sona alinca kopya geri geliyor.
 *  - Sifirlama dongusune AYRI sayac yereli: 132 bayt, 20 fark. Kardeste de
 *    ayni sinif sorun cikmisti (orada 6 fark): ayri allocno onceligi
 *    yukseltip r0/r1 takasina yol aciyor. Iki ic dongunun TEK sayac
 *    degiskenini paylasmasi gerekiyor.
 *  - `u16 remain` (kirpmayi tipe birakmak): 140 bayt, 50 fark. Her
 *    `remain--` sonrasi lsls#16/lsrs#16 ekleniyor, ROM'da yok.
 *  - `(u16)` kirpmasini tumden atmak: 128 bayt (4 eksik). Kirpma gercekten
 *    kaynakta, tipte degil.
 *  - `int n`: 128 bayt. 0x8023BF0'daki daraltma cifti kayboluyor.
 *  - `int stride` (yani [sp,#28] genis parametre): 118 bayt. Girisdeki
 *    normalizasyon dortlusu ile birlikte 14 bayt gidiyor (kural 15).
 *
 * ESDEGER OLCULEN YAZIMLAR (ikisi de birebir eslesiyor, tercih uslup):
 *  - `*p++ = *q++;` tek deyim olarak -- ucluyu acik yazmakla ayni kod.
 *    Kardesle bicim birligi icin acik yazim tercih edildi.
 *  - `q = dest + n;` (p yerine dest tabani) -- o noktada p == dest oldugu
 *    icin CSE ikisini birlestiriyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b6.c  -> 132/132 eslesti
 */

#include "gba_types.h"

/* 0x08023BB8 */
void FUN_08023bb8(u8 *dest, u8 index, u8 rows, u8 length, u8 stride,
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
        p = dest;
        n = *src >> shift;
        src++;
        q = p + n;

        /* Soldaki (length - n) bayti n bayt sagdan cekip sola kaydir.
         * Kirpma acik: bkz. baslik, madde 2. */
        remain = (u16)(length - n);
        while (remain != 0) {
            *p = *q;
            q++;      /* ROM once q'yu artiriyor; sira onemli (madde 4) */
            p++;
            remain--;
        }

        /* Bosalan sagdaki n bayti sifirla. Ayni sayac degiskeni: bkz. baslik. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p++;
            remain--;
        }

        dest += stride;   /* satir ilerlemesi govdenin SONUNDA (madde 5) */
    }
}
