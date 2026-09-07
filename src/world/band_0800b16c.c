/* Dortgen ic sinamasi + en yakin kenarin indeksi - 0x0800B16C (280 bayt)
 *
 * DURUM: BYTE-MATCHING (280/280, fark 0; 140/140 komut).
 *
 * NE YAPIYOR
 * ----------
 * Bir dortgenin dort kenari icin noktanin isaretli determinantini (2B
 * capraz carpim) hesapliyor.  Herhangi bir kenarda deger negatifse nokta
 * disaridadir ve 0 donuyor.  Dortu de gecerse EN KUCUK degeri veren
 * kenarin indeksini bulup `(index << 8) | 1` donuyor: alt bit "iceride",
 * ust bayt "en yakin kenar".  `asrs #12` / `lsls #8` cifti 20.12 sabit
 * noktali aritmetik; prologdaki `(radius >> 12)^2 << 8` bir yaricap
 * karesi ve kenar degerlerine tolerans olarak ekleniyor.  Dortgen
 * kose basina {x, y, z} tutuyor (satir adimi 12 bayt).
 *
 * Kardes dosya: src/world/quad_edge_test.c (0x0800BF18, byte-matching).
 * Ayni dortgen kalibi, ayni kenar sirasi (3, 1, 0, 2), ayni sabit
 * nokta bicimi.  Bu dosya onun "tek nokta + indeks" varyanti.
 *
 * ILK KENARDA `+= radiusSq` YOK - ROM'DA DA YOK
 * ---------------------------------------------
 * ROM 0x0800B1BA'da yalnizca `lsls r0,r2,#8 / cmp r0,#0` var; diger uc
 * kenarda ise `ldr r1,[sp,#0] / adds r0,r0,r1` ile yaricap karesi
 * ekleniyor.  Kaynakta da ilk blokta ekleme YOK.  Kardes dosyadaki
 * dortlu kopyala-yapistir yapisi dusunulunce bu, ozgun koddaki bir
 * unutma; eklendiginde 2 komut fazla cikiyor ve ROM'la uyusmuyor.
 *
 * KAPANISI SAGLAYAN IKI OLCUM
 * ---------------------------
 * (1) YARICAP KARESI TEK IFADE, ARA DEGISKENSIZ OLMALI  (142 -> 140 komut)
 *     ROM: asrs r2,r2,#12 / adds r0,r2,#0 / muls r0,r2 / lsls r0,r0,#8
 *     Yani carpim ve kaydirma AYNI pseudo (yerinde kaydirma), `radius>>12`
 *     ise ayri bir pseudo.  Olculen yazimlar:
 *       side = radius>>12; radiusSq = (side*side)<<8;   -> 111/142
 *          (fazla `adds r0,r2,#0` kopyasi: kaydirma ucuncu bir pseudo)
 *       side = radius>>12; radiusSq = side*side; radiusSq <<= 8;
 *                                                      ->  91/142
 *          (dagitim takla atti: radiusSq sl'ye, minimum sp#0'a - ROM'un TERSI)
 *       radius >>= 12; radiusSq = radius*radius; radiusSq <<= 8;  -> 96/142
 *       radiusSq = (radius>>12)*(radius>>12); radiusSq <<= 8;     -> 96/142
 *       radiusSq = ((radius>>12)*(radius>>12)) << 8;   -> 133/140  DOGRU
 *     Tek ifadede `radius>>12` CSE ile tek `asrs`a iniyor, carpim
 *     sonucu yerinde kaydiriliyor ve radiusSq sp#0 yuvasina, minimum
 *     sl'ye dusuyor - ROM'un dagitimi.
 *
 * (2) `return 0` GOVDESI FONKSIYONUN SONUNDA OLMALI  (kural 49; 133 -> 140)
 *     Duz `if (side < 0) return 0;` yaziminda ilk uc kenar capraz
 *     atlama ile tek kuyruga birlesiyor ama DORDUNCUSU satir icinde
 *     kaliyor: `bge / movs r0,#0 / b` (3 komut), ROM'da ise tek `blt`.
 *     Dorduncu blogun basari yolu once yazilirsa kuyruk sona kayiyor.
 *     Kardes dosyadaki `} else return 0;` bicimi bunu sagliyor.
 *
 * ELENEN / ESDEGER YAZIMLAR
 * -------------------------
 * Kuyruk yerlesimi icin OLCULEN ve HEPSI BYTE-MATCHING cikan bicimler:
 *   - dort blogun da `} else return 0;` ile sarilmasi  (secilen; kardes
 *     dosya ile ayni bicim)
 *   - yalniz son bir / iki / uc blogun sarilmasi
 *   - `goto outside;` + fonksiyon sonunda `outside: return 0;`
 * Bunlar ciktida ayirt edilemiyor; kardes dosyayla tutarli olan secildi.
 * ELENEN (eslesmeyen): dort blokta da duz `if (side < 0) return 0;`
 * (133/140), ve yukarida (1)'de listelenen dort yaricap yazimi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_0800b16c.c
 */

#include "gba_types.h"

s32 FUN_0800b16c(const s32 quad[4][3], const s32 *point, s32 radius)
{
    s32 radiusSq;
    s32 minimum;
    s32 side;
    s32 index;

    /* Tek ifade olmak ZORUNDA; bkz. basliktaki olcum (1). */
    radiusSq = ((radius >> 12) * (radius >> 12)) << 8;
    minimum = 0x1F400000;
    index = 0;

    /* Kenar 3->0.  ROM burada yaricap karesini EKLEMIYOR. */
    side = ((((point[1] - quad[3][1]) >> 12) * ((quad[0][0] - quad[3][0]) >> 12) - ((point[0] - quad[3][0]) >> 12) * ((quad[0][1] - quad[3][1]) >> 12)) << 8);
    if (side >= 0) {
        if (side < minimum) {
            minimum = side;
            index = 3;
        }

        /* Kenar 1->2 */
        side = ((((point[1] - quad[1][1]) >> 12) * ((quad[2][0] - quad[1][0]) >> 12) - ((point[0] - quad[1][0]) >> 12) * ((quad[2][1] - quad[1][1]) >> 12)) << 8);
        side += radiusSq;
        if (side >= 0) {
            if (side < minimum) {
                minimum = side;
                index = 1;
            }

            /* Kenar 0->1 */
            side = ((((point[1] - quad[0][1]) >> 12) * ((quad[1][0] - quad[0][0]) >> 12) - ((point[0] - quad[0][0]) >> 12) * ((quad[1][1] - quad[0][1]) >> 12)) << 8);
            side += radiusSq;
            if (side >= 0) {
                if (side < minimum) {
                    minimum = side;
                    index = 0;
                }

                /* Kenar 2->3 */
                side = ((((point[1] - quad[2][1]) >> 12) * ((quad[3][0] - quad[2][0]) >> 12) - ((point[0] - quad[2][0]) >> 12) * ((quad[3][1] - quad[2][1]) >> 12)) << 8);
                side += radiusSq;
                if (side >= 0) {
                    if (side < minimum) {
                        minimum = side;
                        index = 2;
                    }
                    return (index << 8) | 1;
                } else return 0;
            } else return 0;
        } else return 0;
    } else return 0;
}
