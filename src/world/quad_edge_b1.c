/* Cokgen / nokta ic-disinda sinamasi - 0x0800BDF8-0x0800BEBD (198 bayt)
 *
 * DURUM: BYTE-MATCHING (198/198, fark 0).
 *        Onceki durum: 188 bayt, fark 181 (elle sarmalamali calisma kopyasi).
 *
 * !!! DERLEME ONKOSULU !!!
 * ROM 0x0800BE6E'de `bl 0x0806C18C` yapiyor; orasi agbcc'nin Thumb
 * __modsi3 rutini (0x0806C188'deki `mov pc,lr` onun sifira-bolme
 * kuyrugu).  Yani `%` operatoru bu fonksiyonda ZORUNLU.  data/functions.csv
 * bu adresi yalnizca __modsi3 adiyla taniyor; agbcc'nin urettigi
 * cagri sembolu ise `__modsi3`.  Bu dosyanin linklenmesi icin
 * ***0x0806C18C ADRESI ICIN `__modsi3` ADLI BIR functions.csv KAYDI
 * GEREKIYOR*** (mevcut __modsi3 satirinin adi degistirilebilir ya da
 * takma ad eklenebilir).  Kayit olmadan build_c.py
 * "'__modsi3' data/functions.csv veya data/ram_map.csv'de yok" diyor.
 * Kaydi ben eklemedim; data/ altina dokunmak bu goreve kapali.
 * Olcum, bellekte gecici olarak bu satiri ekleyen bir yardimci ile
 * yapildi ve 198 baytin TAMAMI ROM ile ayni cikti.
 *
 * NE YAPIYOR
 * ----------
 * Kapali bir cokgenin (`count` koseli, kose basina 20.12 sabit noktali
 * {x, y, z}) icinde `point` var mi diye bakiyor.
 *   1. Once kenar kenar gezip son kenarin iki ucundaki z degerlerinden
 *      buyugunu maxZ'ye, kucugunu minZ'ye koyuyor.  Dongu her adimda
 *      ustune yazdigi icin geriye SON kenarin degerleri kaliyor; ROM'da
 *      da boyle, yani bu bir yukseklik araligi biriktirmesi degil, son
 *      kenarin z sinirlari.
 *   2. `tolerance << 7` payiyla z araligini test ediyor; disarda kalirsa 0.
 *   3. Her kenar icin 2B capraz carpim (isaretli alan) hesaplayip
 *      `<< 8` ile olcekliyor ve tolerans ekliyor; negatif cikan ilk
 *      kenarda 0 donuyor.  Hepsi gecerse 1.
 * `>> 12` / `<< 8` cifti kardes dosyadaki (quad_edge_test.c) ile ayni
 * 20.12 sabit noktali kaliptir.
 *
 * KAPANISI SAGLAYAN IKI OLCUM
 * ---------------------------
 * (1) `%` AYRI BIR DEYIME ALINMAMALI, IFADENIN ICINDE KALMALI
 *     (fark 127 -> 2; ikinci dongu bayt bayt ROM ile ayni oldu)
 *     `j = (i + 1) % count;` yazimi cagriyi dongu govdesinin BASINA
 *     tasiyor.  ROM'da cagri ortada:
 *       ldr r5,[point,#4] / ldr r3,[poly_i,#4] / mov r9,r3
 *       subs r5,r5,r3 / asrs r5,#12      <- A once hesaplaniyor
 *       adds r6,r0,#1 / adds r0,r6,#0 / mov r1,r8 / bl __modsi3
 *     Yani `(point[1]-poly[i][1])>>12` cagridan ONCE uretiliyor; bu
 *     ancak modulo carpimin SAG operandinin icindeyken olur.
 *     Zincirleme etkisi dagitima kadar iniyor: A cagriyi asmak zorunda
 *     oldugu icin poly[i][1] callee-saved r9'u tutuyor, bu da `point`i
 *     yazmactan atip sp#4 yuvasina dusuruyor ve `tolerance` sl'ye
 *     yerlesiyor.  ROM'un `sub sp,#8` + `str r2,[sp,#4]` prologu tam
 *     olarak bu.  Ayri deyimli halde `sub sp,#4` cikiyordu.
 *     Iki ayri `(i + 1) % count` yazimi CSE ile tek cagriya iniyor
 *     (libcall const kabul ediliyor), ROM'da da tek `bl` var.
 *
 * (2) BIRINCI DONGUDE SATIR ISARETCILERI SART   (fark 2 -> 0)
 *     Dogrudan `poly[i][2]` / `poly[i+1][2]` yazildiginda gcc'nin
 *     dongu-guclendirmesi taban givi +8 ile ONYUKLUYOR:
 *       ldr r3,[sp,#0] / adds r3,#8 / ldr r2,[r3,#0] / ldr r1,[r3,#12]
 *     ROM ise tabani ONYUKSUZ tutup ofsetleri mem'de birakiyor:
 *       ldr r3,[sp,#0] / ldr r2,[r3,#8] / ldr r1,[r3,#20]
 *     Fark tam 2 bayt.  Taban givin add_val'inin 0 olmasi icin dongude
 *     add_val = 0 olan bir giv BULUNMASI gerekiyor; `poly[i]` satir
 *     adresini bir yerele almak (`cur = poly[i];`) o givi uretiyor,
 *     `next = poly[i+1];` de onunla birlesip +12 ofsetine katlaniyor.
 *     Boylece `[r3,#8]` ve `[r3,#20]` cikiyor, `adds r3,#8` kayboluyor.
 *
 * ELENEN YOLLAR - BUNLARI TEKRAR DENEMEYIN
 * ----------------------------------------
 * Onceki oturumdan devralinan not (DUZELTILDI, yanlisti):
 *   - "`%` OPERATORU YASAK: agbcc __modsi3'e ceviriyor, sembol yok,
 *     dosya HIC derlenmiyor" -> sembolun YOKLUGU bir csv eksigiydi,
 *     dilin kisiti degil.  Elle sarmalama (`j = i+1; if (j>=count) j=0;`)
 *     188 bayt / fark 181 veriyordu; ROM gercekten cagri yapiyor.
 * Bu oturumda olculen ve elenen yollar:
 *   - `j = (i + 1) % count;` ayri deyim: 196 bayt, fark 127.
 *     Prolog `sub sp,#4` ile ciktigi icin dagitim bastan sapiyor.
 *   - Birinci donguyu tek satir isaretcisiyle yazmak (`a[2]` ve `a[5]`):
 *     200 bayt, fark 146 -- `a[5]` ayri bir giv uretmiyor, onyukleme
 *     geri geliyor.
 *   - `const s32 (*edge)[3] = &poly[i];` + `edge[0][2]`/`edge[1][2]`:
 *     200 bayt, fark 146.  Satir dizisi isaretcisi giv uretmiyor.
 *   - Isaretcileri bildirimde ilklendirmek ile ayri atamak arasinda
 *     FARK YOK (ikisi de 0); okunakli olan secildi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/quad_edge_b1.c
 *             (yukaridaki __modsi3 kaydi eklendikten sonra)
 */

#include "gba_types.h"

s32 IsPointInPolygon(const s32 poly[][3], u16 count, const s32 *point, s32 tolerance)
{
    s32 maxZ;
    s32 minZ;
    s32 i;
    s32 side;

    maxZ = 0;
    minZ = 0;
    tolerance <<= 7;

    /* Kenar kenar gezip SON kenarin z sinirlarini birakiyor: dongu her
     * adimda maxZ/minZ'nin ustune yaziyor, biriktirme yok.  ROM ayni. */
    for (i = 0; i < count - 1; i++) {
        const s32 *cur;
        const s32 *next;

        /* Satir adresleri YEREL OLMAK ZORUNDA: dogrudan poly[i][2]
         * yazimi taban givi +8 onyukluyor ve 2 fazla bayt uretiyor
         * (basliktaki olcum 2). */
        cur = poly[i];
        next = poly[i + 1];
        if (cur[2] > next[2]) {
            maxZ = cur[2];
            minZ = next[2];
        } else {
            maxZ = next[2];
            minZ = cur[2];
        }
    }

    /* Yukseklik penceresi disindaysa kenarlara hic bakilmiyor. */
    if (maxZ < point[2] - tolerance) return 0;
    if (minZ > point[2] + tolerance) return 0;

    for (i = 0; i < count; i++) {
        /* `(i + 1) % count` IFADENIN ICINDE kalmali: ayri bir deyime
         * alinirsa __modsi3 cagrisi dongu basina kayiyor ve dagitim
         * ROM'dan ayriliyor (basliktaki olcum 1).  Iki yazim CSE ile
         * tek `bl`ye iniyor, ROM'da da tek cagri var. */
        side = ((((point[1] - poly[i][1]) >> 12)
                * ((poly[(i + 1) % count][0] - poly[i][0]) >> 12)
                - ((point[0] - poly[i][0]) >> 12)
                * ((poly[(i + 1) % count][1] - poly[i][1]) >> 12)) << 8);
        side += tolerance;
        if (side < 0) return 0;
    }
    return 1;
}
