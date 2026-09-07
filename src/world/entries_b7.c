/* ShiftColumnsDown -- 0x08023B24-0x08023BB7 (148 bayt)
 *
 * Sutun sutun ASAGI kaydirma. Kaynak tamponundan her sutun icin bir bayt
 * okuyup `shift` kadar saga kaydiriyor; cikan `n` degeri o sutunun kac
 * satir asagi kayacagini soyluyor. Hedefte satirlar `stride` bayt arayla
 * duruyor, sutundan sutuna 1 bayt ilerleniyor. Yurutucu sutunun EN ALT
 * satirindan basliyor (dest + (height-1)*stride), (height - n) satiri
 * yukaridan asagi cekiyor, sonra ustte bosalan n satiri sifirliyor.
 *
 * AILE ILISKISI -- ucuzun ortasi:
 *
 *               sutun ici adim   sutundan sutuna   yon
 *   b3 (c3c)    +stride          +1                yukari (p artiyor)
 *   b6 (bb8)    +1               +stride           sola   (p artiyor)
 *   b7 (b24)    -stride          +1                asagi  (p AZALIYOR)
 *
 * b3'un ayna esi. Kontrol akisini yine de kardesten kopyalamadim, ROM'dan
 * okudum (docs/COMPILER.md kural 49 uyarisi); burada bir sey de degisiyor,
 * asagida madde 1.
 *
 * Imza ROM'dan okundu, tahmin degil:
 *   r0            -> dest   (isaretci, normalize edilmiyor)
 *   r1,r2,r3      -> lsls#24/lsrs#24 cifti var => UCU DE u8
 *   [sp,#32]      -> ayni normalizasyon => u8 (stride)
 *   [sp,#36]      -> normalizasyon yok, sifirla karsilastiriliyor => isaretci
 *   [sp,#40]      -> her turda yeniden okunuyor, asrs ile kullaniliyor => int
 * Prolog `push {r4-r7,lr}` + `mov r7,sl` / `mov r6,r9` / `mov r5,r8` +
 * `push {r5,r6,r7}`: r8/r9/r10 da kullaniliyor, yani SEKIZ callee-saved.
 * Kardes b3'te yedi vardi; farki madde 1 acikliyor. Yigin arguman
 * ofsetlerinin 32/36/40 olmasi (b3'te 28/32/36) dogrudan bunun sonucu --
 * ofsetleri kardesten kopyalayan biri burada yanlis parametreleri okur.
 * Donus `pop {r0}; bx r0` ve r0 olu => void (kural 35).
 *
 * ROM'dan OLCULEN ayrintilar:
 *
 *  1. 0x8023B54-5C dis dongu govdesinden ONCE bir kez calisiyor:
 *     `mov r0,r8` / `subs r0,#1` / `adds r1,r0,#0` / `muls r1,r5` ve sonuc
 *     sl'de bekliyor. Yani (height - 1) * stride dongu-degismezi olarak
 *     hoist edilmis. Kaynakta ifade dongunun ICINDE yazili; agbcc kendisi
 *     disari cikariyor. Sekizinci callee-saved yazmacin (sl) sebebi bu
 *     carpimin butun dongu boyunca canli kalmasi.
 *  2. `n` u8: 0x8023B68'deki lsls#24/lsrs#24 cifti. Kaydirma `asrs`
 *     (aritmetik) cunku ldrb'nin sonucu int'e yukseliyor ve `shift` int.
 *  3. Kopyalanacak satir sayisi 0x8023B78'de lsls#16/lsrs#16 ile 16 bite
 *     kirpiliyor AMA dongu icindeki azaltma duz `subs r1,#1` -- kirpma
 *     tekrarlanmiyor. Degisken u16 DEGIL; genis bir yerele yazilmis
 *     (u16) donusumu (olcum icin asagiya bak).
 *  4. Dis dongu sayaci `i` isaretsiz: 0x8023B52 `bcs` ve 0x8023BA8 `bcc`
 *     (kural 31). `int i` yazimi `bge`/`blt` verirdi.
 *  5. Yurume yonu azaliyor: `subs r2,r2,r5` / `subs r3,r3,r5`, yani
 *     p -= stride. Kaynak isaretcisi q = p - n*stride
 *     (0x8023B72 `subs r3,r2,r0`).
 *  6. `dest++` (0x8023B7C-7E `movs r0,#1` / `add ip,r0`) ve `i++` ic
 *     dongulerden ONCE cikiyor. Bu agbcc'nin kendi tasimasi, kaynagin
 *     bicimi degil; artirim govdenin SONUNDA (olculdu, asagida).
 *
 * DENENIP ELENEN YAZIMLAR (hepsi 148 bayt hedefine karsi olculdu):
 *
 *  - `u16 remain` (kirpmayi tipe birakmak): 156 bayt, 50 fark. Her
 *    `remain--` sonrasi lsls#16/lsrs#16 ekleniyor, ROM'da yok. Genis
 *    yerel + acik (u16) donusumu sart (madde 3).
 *  - Sifirlama dongusune AYRI sayac (`u32 rows; rows = n; ... rows--`):
 *    148 bayt, 6 fark. Ayri yerel ikinci bir allocno uretip r0'i kapiyor
 *    ve sifir sabitini yerinden ediyor; ROM'da iki sayac da r1, sifir
 *    sabiti r0. Iki ic dongu TEK sayac degiskenini paylasmali. Kural
 *    50'nin tersten uygulanisi: bolmek degil, BIRLESTIRMEK.
 *  - `dest++` govde ORTASINDA (q hesabindan hemen sonra): 148 bayt,
 *    12 fark.
 *  - `for (i = 0; i < columns; i++, dest++)`: 148 bayt, 5 fark.
 *    Kural 43'un bicimi burada TUTMUYOR. ROM'un erken ip artirimi
 *    kaynakta for artirimindan degil, govde sonundaki ayri deyimden
 *    cikiyor -- derleyici onu kendisi one aliyor.
 *
 * OLCUM SIRASINDA YASANAN CAKISMA (kayit icin): bu dosya yolu olcum
 * ortasinda baska bir ajan tarafindan uzerine yazildi ve iki varyant
 * kosusu (dest + height*stride - stride ile genis n) sanki 0x08023BB8'i
 * eslestiriyormus gibi gorundu. Oyle degil: derlenen sey o ajanin
 * ShiftRowsLeft taslagiydi. O iki varyant BURADA gecerli bir olcum DEGIL,
 * tekrar denenmeleri gerekir; yanlis kayit birakmamak icin yukaridaki
 * elenenler listesine almadim.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b7.c  -> 148/148 eslesti
 */

#include "gba_types.h"

/* 0x08023B24 */
void ShiftColumnsDown(u8 *dest, u8 index, u8 columns, u8 height, u8 stride,
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
    for (i = 0; i < columns; i++) {
        /* Sutunun en alt satiri. Carpimi agbcc dongu disina tasiyor. */
        p = dest + (height - 1) * stride;
        n = *src >> shift;
        src++;
        q = p - n * stride;

        /* Alttaki (height - n) satiri n satir yukaridan cekip asagi kaydir. */
        remain = (u16)(height - n);
        while (remain != 0) {
            *p = *q;
            p -= stride;
            q -= stride;
            remain--;
        }

        /* Bosalan ustteki n satiri sifirla. Ayni sayac degiskeni: bkz. baslik. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p -= stride;
            remain--;
        }

        dest++;
    }
}
