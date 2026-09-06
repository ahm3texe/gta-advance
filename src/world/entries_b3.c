/* FUN_08023c3c — 0x08023C3C-0x08023CC1 (134 bayt)
 *
 * Sutun sutun yukari kaydirma. Kaynak tamponundan her sutun icin bir bayt
 * okuyup `shift` kadar saga kaydiriyor; cikan `n` degeri o sutunun kac
 * satir yukari kayacagini soyluyor. Hedefte satirlar `stride` bayt arayla
 * duruyor (yani hedef sutun-icinde stride adimliyor, sutundan sutuna 1
 * bayt ilerliyor). Once (height - n) satir asagidan yukari kopyalaniyor,
 * sonra altta kalan n satir sifirlaniyor. Klasik "cubuk/dalga" cizimi.
 *
 * Imza ROM'dan okundu, tahmin degil:
 *   r0            -> dest   (isaretci, normalize edilmiyor)
 *   r1,r2,r3      -> lsls#24/lsrs#24 cifti var => UCU DE u8
 *   [sp,#28]      -> ayni normalizasyon => u8 (stride)
 *   [sp,#32]      -> normalizasyon yok, sifirla karsilastiriliyor => isaretci
 *   [sp,#36]      -> her turda yeniden okunuyor, asrs ile kullaniliyor => int
 * Prolog `push {r4-r7,lr}` + `mov r7,r9`/`mov r6,r8`/`push {r6,r7}`, yani
 * r8/r9 da kullaniliyor; yedi canli deger var (docs/COMPILER.md register
 * tablosu). Donus `pop {r0}; bx r0` ve r0 olu => void (kural 35).
 *
 * ROM'dan OLCULEN ayrintilar ve neden oyle yazildigi:
 *
 *  1. `n` u8: 0x8023C72'deki lsls#24/lsrs#24 cifti. Kaydirma `asrs`
 *     (aritmetik) cunku ldrb'nin sonucu int'e yukseliyor ve `shift` int.
 *  2. Kopyalanacak satir sayisi 0x8023C82'de lsls#16/lsrs#16 ile 16 bite
 *     kirpiliyor AMA dongu icindeki azaltma duz `subs r1,#1` — kirpma
 *     tekrarlanmiyor. Yani degisken u16 DEGIL; genis bir yerele yazilmis
 *     `(u16)` donusumu. `u16 remain` yazimi her azaltmadan sonra fazladan
 *     lsls/lsrs cifti uretiyor (olculdu: 33/134 fark).
 *  3. Dis dongu sayaci `i` isaretsiz: 0x8023C68 `bcs` ve 0x8023CB4 `bcc`
 *     (kural 31). `int i` yazimi `bge`/`blt` verirdi.
 *  4. Sutun ilerlemesi govdenin SONUNDA `dest++`. ROM p+1'i erken hesaplayip
 *     `ip`'te bekletiyor (0x8023C86-88) ve dongu dibinde `mov r4, ip` ile
 *     geri yaziyor — bu, kaynakta artirimi SONA koyunca kendiliginden
 *     cikiyor, `dest = p + 1;` diye ortada yazinca cikmiyor (bkz. elenenler).
 *
 * DENENIP ELENEN YAZIMLAR (en degerli kisim, ayni duvara toslamayin):
 *
 *  - `u16 remain` (kirpmayi tipe birakmak): 33/134 fark. Her `remain--`
 *     sonrasi lsls#16/lsrs#16 ekleniyor, ROM'da yok. Genis yerel + acik
 *     `(u16)` donusumu gerekiyor (yukarida madde 2).
 *  - Sutun ilerlemesinin yeri, alti varyant olculdu (hepsi 134 hedefine
 *     karsi):
 *        `dest = p + 1;` govde ortasinda      -> 130 bayt (4 eksik)
 *        `dest++;` govde ortasinda            -> 130 bayt
 *        `dest = p + 1;` govde sonunda        -> 126 bayt
 *        `for (i = ...; i++, dest++)`         -> 134, fark 12
 *        ayri `next = p + 1; ... dest = next` -> 134, fark  6
 *        `dest++;` govde SONUNDA              -> 134, fark  6  (dogru bicim)
 *     Ortada yazilan biciminde agbcc `dest`i dogrudan `ip`'e dagitip
 *     dongu dibindeki `mov r4, ip` kopyasini eliyor; sona alinca `dest`
 *     r4'te kaliyor ve ROM'un kopyasi cikiyor.
 *  - Sifirlama dongusunun sayaci icin ayri bir yerel (`rows = n; while
 *     (rows != 0) ...`): 134 bayt, 6 fark — tam olarak r0/r1 takasi.
 *     `dump_alloc.py` sebebi gosterdi: ayri yerel 13 referans / 10 omur ile
 *     3.900 onceliige cikip SIRA 1 oluyor ve en kucuk bos yazmaci, r0'i
 *     kapiyor; kopyalama sayaci (3*13/13 = 3.000) sonra gelip r1'i aliyor.
 *     ROM'da ikisi de r1, sifir sabiti r0. Ayni ayri-yerel fikrinin
 *     denenen butun cesitleri de 6 farkta kaldi: bildirim sirasini
 *     degistirmek, tipini `int` yapmak, `for (rows = n; rows != 0; rows--)`
 *     yazmak. `u16 rows` 138 bayta cikti (kirpma), `n`'i dogrudan sayac
 *     yapmak 136 (u8 azaltma kirpmasi).
 *  - Sifir sabitini yerele almak (`zero = 0; *p = zero;`): 5 fark. Sabiti
 *     tasimak yanlis kaldirac; asil sorun sayacin ONCELIGIydi.
 *
 * COZUM: iki ic dongu TEK sayac degiskenini paylasiyor. Boylece iki ayri
 * allocno yerine tek allocno olusuyor, r1'e dusuyor ve sifir sabiti r0'da
 * kaliyor — ROM'un dagitimi. Kural 50'nin tersten uygulanisi: degiskeni
 * bolmek yerine BIRLESTIRMEK gerekti.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b3.c  -> 134/134 eslesti
 */

#include "gba_types.h"

/* 0x08023C3C */
void FUN_08023c3c(u8 *dest, u8 index, u8 columns, u8 height, u8 stride,
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
        p = dest;
        n = *src >> shift;
        src++;
        q = p + n * stride;

        /* Ustteki (height - n) satiri n satir asagidan cekip yukari kaydir. */
        remain = (u16)(height - n);
        while (remain != 0) {
            *p = *q;
            p += stride;
            q += stride;
            remain--;
        }

        /* Bosalan alttaki n satiri sifirla. Ayni sayac degiskeni: bkz. baslik. */
        remain = n;
        while (remain != 0) {
            *p = 0;
            p += stride;
            remain--;
        }

        dest++;
    }
}
