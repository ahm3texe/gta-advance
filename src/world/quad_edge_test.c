/* Dort kenar / iki nokta sinamasi — 0x0800BF18-0x0800C121 (522 bayt)
 *
 * Dortgenin dort kenari icin iki noktanin isaretli determinantini (2B
 * capraz carpim) hesapliyor.  Ilk noktanin herhangi bir kenardaki degeri
 * negatifse hemen 0 donuyor.  Ikinci noktanin negatif degerlerinden EN
 * KUCUGUNU izleyip o kenarin iki ucunu dort cikis isaretcisine yaziyor.
 *
 * `asrs #12` / `lsls #8` ciftleri 20.12 sabit noktali aritmetigi
 * gosteriyor.  Prologdaki `(radius >> 12)^2 << 8` bir yaricap karesi.
 *
 * DURUM: PARK — 72/522 fark.  BOYUT DOGRU (522/522) ve anlambilim
 * cozulmus durumda; kalan fark tamamen YAZMAC DAGITIMI.
 *
 * Kalan farkin sekli (18 kume, 49 yarim-kelime; en buyugu +0x01A..+0x04A):
 *   ROM  : mov ip,r1   (2. parametre ip'de)   /  mov r9,r0  (radiusSq r9'da)
 *   bizim: mov r9,r1   (2. parametre r9'da)   /  mov ip,r2  (radiusSq ip'de)
 * Ayrica ROM yaricap karesini YERINDE kaydirmiyor, once kopyaliyor:
 *   ROM  : adds r0,r2,#0 / lsls r0,#8 / mov r9,r0
 *   bizim: lsls r2,r2,#8 / mov ip,r2
 *
 * ELENEN YOLLAR (632 varyant olculdu, hepsi >= 72 fark):
 *   - Parametre tipleri: s32[4][3], duz s32*, struct Vec*, struct Quad*
 *   - Determinant yazimi: satir ici ifade, ara degiskenler, inline
 *     yardimci fonksiyon (dort ayri govde bicimiyle)
 *   - Yaricap karesi: dort ayri yazim, uc ayri hedef degisken
 *   - Yerel omru: fonksiyon govdesinde, blok icinde, kenar basina ayri
 *   - Kontrol akisi: erken donus zinciri vs ic ice "gecerse devam et"
 *     (ic ice bicim 522 baytin altina inen tek yol oldu)
 *   - decomp-permuter iki ayri tabandan kosturuldu; skor binlerde kaldi,
 *     yakinsamadi
 *   - "Carpim ve kaydirmayi AYRI degiskenlere ayir" hipotezi: 483 farka
 *     firladi ve cikti 518 bayta dustu -- agbcc fazla yerelleri tamamen
 *     eledi.  HIPOTEZ CURUTULDU.
 *
 * dump_alloc.py: 120 sozde-yazmac, 69 SPILL.  En yuksek oncelikli pseudo
 * 36 (26 referans, 48 omur, oncelik 2.167).  Bu yogunlukta kaynak
 * duzeyinden yazmac secimini yonlendirmenin bilinen bir yolu yok.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/quad_edge_test.c
 */

#include "gba_types.h"

s32 FUN_0800bf18(const s32 quad[4][3], const s32 *from, const s32 *to,
                  s32 radius, s32 height, s32 *outX1, s32 *outY1,
                  s32 *outX2, s32 *outY2)
{
    s32 radiusSq;
    s32 minimum;
    s32 scaled, squared;
    s32 fromSide, toSide;

    toSide = radius >> 12;
    radiusSq = toSide * toSide;
    radiusSq = radiusSq << 8;
    minimum = 0x00FFFFFF;
    if (quad[0][2] <= from[2] + height) {
        fromSide = ((((from[1] - quad[3][1]) >> 12) * ((quad[0][0] - quad[3][0]) >> 12) - ((from[0] - quad[3][0]) >> 12) * ((quad[0][1] - quad[3][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[3][1]) >> 12) * ((quad[0][0] - quad[3][0]) >> 12) - ((to[0] - quad[3][0]) >> 12) * ((quad[0][1] - quad[3][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[3][0];
            *outY1 = quad[3][1];
            *outX2 = quad[0][0];
            *outY2 = quad[0][1];
        }
        fromSide = ((((from[1] - quad[1][1]) >> 12) * ((quad[2][0] - quad[1][0]) >> 12) - ((from[0] - quad[1][0]) >> 12) * ((quad[2][1] - quad[1][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[1][1]) >> 12) * ((quad[2][0] - quad[1][0]) >> 12) - ((to[0] - quad[1][0]) >> 12) * ((quad[2][1] - quad[1][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[1][0];
            *outY1 = quad[1][1];
            *outX2 = quad[2][0];
            *outY2 = quad[2][1];
        }
        fromSide = ((((from[1] - quad[0][1]) >> 12) * ((quad[1][0] - quad[0][0]) >> 12) - ((from[0] - quad[0][0]) >> 12) * ((quad[1][1] - quad[0][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[0][1]) >> 12) * ((quad[1][0] - quad[0][0]) >> 12) - ((to[0] - quad[0][0]) >> 12) * ((quad[1][1] - quad[0][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[0][0];
            *outY1 = quad[0][1];
            *outX2 = quad[1][0];
            *outY2 = quad[1][1];
        }
        fromSide = ((((from[1] - quad[2][1]) >> 12) * ((quad[3][0] - quad[2][0]) >> 12) - ((from[0] - quad[2][0]) >> 12) * ((quad[3][1] - quad[2][1]) >> 12)) << 8);
        fromSide += radiusSq;
        toSide = ((((to[1] - quad[2][1]) >> 12) * ((quad[3][0] - quad[2][0]) >> 12) - ((to[0] - quad[2][0]) >> 12) * ((quad[3][1] - quad[2][1]) >> 12)) << 8);
        toSide += radiusSq;
        if (fromSide >= 0) {
        if (toSide < minimum && toSide < 0) {
            minimum = toSide;
            *outX1 = quad[2][0];
            *outY1 = quad[2][1];
            *outX2 = quad[3][0];
            *outY2 = quad[3][1];
        }
        return 1;
    }
    }
    }
    }
    }
    return 0;
}
