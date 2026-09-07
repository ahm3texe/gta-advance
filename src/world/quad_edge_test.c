/* Dort kenar / iki nokta sinamasi - 0x0800BF18-0x0800C121 (522 bayt)
 *
 * DURUM: BYTE-MATCHING (522/522, fark 0).  Onceki durum 72 farkti.
 *
 * NE YAPIYOR
 * ----------
 * Dortgenin dort kenari icin iki noktanin isaretli determinantini (2B
 * capraz carpim) hesapliyor.  Ilk noktanin herhangi bir kenardaki degeri
 * negatifse hemen 0 donuyor.  Ikinci noktanin negatif degerlerinden EN
 * KUCUGUNU izleyip o kenarin iki ucunu dort cikis isaretcisine yaziyor.
 * `asrs #12` / `lsls #8` ciftleri 20.12 sabit noktali aritmetigi
 * gosteriyor.  Prologdaki `(radius >> 12)^2 << 8` bir yaricap karesi.
 *
 * KAPANISI SAGLAYAN UC OLCUM
 * --------------------------
 * (1) YARICAP KARESI TEK IFADE OLMALI   (fark 72; yazmac takasi cozuldu)
 *     ROM: adds r0,r2,#0 / lsls r0,#8 / mov r9,r0   <- ARADA KOPYA VAR.
 *     Bu kopya, carpim sonucunun ve kaydirma sonucunun AYRI iki pseudo
 *     oldugunun imzasi.  `radiusSq = X; radiusSq = radiusSq << 8;` tek
 *     pseudo uretip yerinde kaydiriyordu (lsls r2,r2,#8).
 *     `radiusSq = (toSide * toSide) << 8;` yazimi ikinci pseudoyu uretti.
 *     Kural 50 hesabi (tools/dump_alloc.py ile dogrulandi):
 *       once : p31 radiusSq 11 ref /162 omur = 0.204  ->  ip (r12)
 *              p23 from     10 ref /151 omur = 0.199  ->  r9
 *       sonra: radiusSq 9 ref /160 omur = 0.169, from 0.199
 *              SIRA TERSINDI: from -> ip, radiusSq -> r9, to -> r10.
 *     agbcc dagitim sirasi: r0..r7, sonra r12(ip), sonra r8/r9/r10.
 *
 * (2) EN DIS `if` ERKEN DONUSE CEVRILMELI   (fark 461 -> 11, boyut 522)
 *     Ic ice bicimde reload, prologdaki `mov r4,ip` kopyasini yukseklik
 *     dalinin OTESINE tasiyip ilk kenarda tekrar kullaniyordu; bu da
 *     r7'yi bosta birakip quad[0][0] gecicisini r7'ye atiyor, fazladan
 *     bir `mov r1,r8` dogurup fonksiyonu 1 komut buyutuyordu.
 *     `if (quad[0][2] > from[2] + height) return 0;` yaziminda reload
 *     kenar blogunda TAZE bir `mov r7,ip` uretiyor; r7 dolu oldugu icin
 *     gecici r1'e dusuyor ve fazla komut kayboluyor.  ROM ile birebir.
 *
 * (3) GCSE HASH TABLOSU BOYUTU   (fark 11 -> 0)
 *     Kalan 11 bayt tamamen YIGIN YUVASI numaralamasiydi:
 *       benim: sp#8=quad[3][0] sp#12=quad[3][1] sp#16=quad[0][0]
 *       ROM  : sp#8=quad[0][0] sp#12=quad[3][0] sp#16=quad[3][1]
 *     Yuvalar reload'da ARTAN PSEUDO NUMARASI sirasiyla dagitiliyor.
 *     Bloklar arasi kose degerlerinin pseudolarini (212..219) GCSE
 *     uretiyor; numaralari ifade HASH KOVASI sirasindan geliyor:
 *       kova = (hash_taban + bayt_ofseti) mod S,  S = (komut_sayisi/4)|1
 *     `quad[0][0]` ofsetsiz oldugu icin adresi duz `(reg 22)`; hash'i
 *     digerlerinden bagimsiz ve kovasi S ile birlikte kayiyor.
 *     OLCULEN HARITA (gcse girisindeki komut sayisi -> kose sirasi):
 *       188..195 -> 16,24,28,36,40,0,4,12   (benim eski halim)
 *       196..198 -> 12,16,24,28,0,36,40,4   (ROM)
 *       200..204 -> baska siralar
 *     Yani gcse'ye 188 yerine 196 komutla girmek gerekiyordu.  Cikti
 *     kodunu HIC degistirmeden komut eklemenin tek yolu, jump2'nin
 *     CAPRAZ ATLAMA (cross-jumping) ile birlestirdigi TEKRARLI
 *     KUYRUKLAR; cross-jumping gcse'den SONRA calisiyor:
 *       - dort kenar kapanisini `} else return 0;` yapmak: +2 komut
 *         adet basina, cikti bayt bayt AYNI (dortte doyuyor, +6)
 *       - son min blogunun icine ikinci bir `return 1;` koymak: +2
 *     Toplam 188 + 8 = 196.  Fark 0.
 *
 * ELENEN YOLLAR - BUNLARI TEKRAR DENEMEYIN
 * ----------------------------------------
 * Onceki oturumlardan (632 varyant, hepsi >= 72 fark):
 *   - Parametre tipleri: s32[4][3], duz s32*, struct Vec*, struct Quad*
 *   - Determinant yazimi: satir ici, ara degiskenler, inline yardimci
 *   - Yaricap karesi: dort ayri yazim, uc ayri hedef degisken
 *   - decomp-permuter iki ayri tabandan; skor binlerde kaldi
 *   - "Carpim ve kaydirmayi ayri degiskenlere ayir": 483 farka firladi
 * Bu oturumda olculen ve ELENEN yollar:
 *   - Yukseklik testinin bicimi (>=, !(>), gecici degisken, - height):
 *     hicbiri yazmac dagitimini oynatmadi.
 *   - `radiusSq` icin `scaled`/`squared` gibi BASKA bir adi tekrar
 *     kullanmak: cikti 518 bayta dusuyor.  `toSide`nin radius>>12'yi
 *     tutmasi VE ayri bir kaydirma pseudosu olmasi sart.
 *   - Carpim operandlarini ters cevirmek (b*a): 522/167 fark.  GCSE
 *     kova sirasini HIC etkilemiyor.
 *   - Kenar govdelerini ara degiskenlere bolmek: 530-546 bayt.
 *   - Kenar basina ayri fromSide/toSide yerelleri (kural 45): 518-546.
 *   - toSide'i fromSide'dan once hesaplamak: 530+ bayt.
 *   - OLU KOD ile komut sayisi buyutme (olu kopya, olu yukleme, olu
 *     carpim, olu kaydirma, olu bellek okumasi - 10 ayri kalip): cse1
 *     hepsini gcse'den ONCE siliyor, komut sayisi 188'de kaliyor.
 *     BU YOL KAPALI.
 *   - Ic `if (fromSide >= 0) {` gatelerini DUZ erken donuse cevirmek:
 *     yalniz 2. sirada zararsiz; 1., 3., 4. sirada 526-558 bayt.
 *     `} else return 0;` bicimi ise DORDUNDE de ciktiyi bozmuyor;
 *     kullanilan bu.
 *   - `&&` yerine ic ice if, ek blok parantezi, donusu degiskene alma,
 *     minimum'u yerele alma, goto: komut sayisini oynatmiyor (goto -2).
 *
 * ARAC NOTU
 * ---------
 * gcse giris komut sayisi olcumu:
 *   old_agbcc <bayraklar> -da -o out.s in.i    ->  in.i.cse dokumunde
 *   `^(insn ` + `^(jump_insn ` satirlarinin sayisi.
 * Web pseudolari (212..219) ve hangi kose ofsetine denk geldikleri
 * in.i.gcse dokumunde su satirlardan okunuyor:
 *   (set (reg:SI N) (mem:SI (plus (reg/v:SI 22) (const_int C))))
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/quad_edge_test.c
 */

#include "gba_types.h"

s32 FindQuadEdgeCrossing(const s32 quad[4][3], const s32 *from, const s32 *to,
                  s32 radius, s32 height, s32 *outX1, s32 *outY1,
                  s32 *outX2, s32 *outY2)
{
    s32 radiusSq;
    s32 minimum;
    s32 fromSide, toSide;

    toSide = radius >> 12;
    radiusSq = (toSide * toSide) << 8;
    minimum = 0x00FFFFFF;
    if (quad[0][2] > from[2] + height) return 0;
    {
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
            return 1;
        }
        return 1;
    } else return 0;
    } else return 0;
    } else return 0;
    } else return 0;
    }
    return 0;
}
