/* Geometri yardimcilari — 0x0800B140 ve 0x0800BDB0
 *
 * Iki bagimsiz kucuk rutin; ROM'da bitisik degiller ama ayni alana ait.
 *
 * 0x0800B140 — nokta carpimi.  Her bilesen carpimdan ONCE `asrs #8` ile
 *   olcekleniyor; 16.16 sabit noktali degerlerin carpiminda tasmayi
 *   onlemek icin klasik kalip.
 *
 * 0x0800BDB0 — kutu yakinlik sinamasi.  Iki noktanin her ekseni arasindaki
 *   MUTLAK farki, yaricaplarin toplamiyla karsilastiriyor; uc eksende de
 *   fark toplam yaricaptan kucukse 1 donuyor.
 *
 * SIRA KANITI (0x0800BDB0): ROM ilk IKI bilesende ara sonucu `a[i]`yi
 * tutan yazmacta (r1) birakiyor, UCUNCUDE r0 kullaniyor:
 *     bilesen 0/1:  ldr r1,[..] / subs r1,r1,r0 / negs r1,r1 / subs r6,r1,r3
 *     bilesen 2  :  ldr r1,[..] / subs r0,r1,r0 / negs r0,r0 / subs r0,r0,r3
 * Bu asimetri kaynaktan geliyor: ilk ikisi PAYLASILAN bir ara degiskenden
 * gecip hedefe yaziliyor, ucuncusu dogrudan hedef degiskende hesaplaniyor.
 * Uc bilesenin de ayni bicimde yazilmasi 8 bayt fark veriyordu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/geometry.c
 */

#include "gba_types.h"

/* 0x0800B140 */
s32 FUN_0800b140(const s32 *a, const s32 *b)
{
    return (a[0] >> 8) * (b[0] >> 8)
         + (a[1] >> 8) * (b[1] >> 8)
         + (a[2] >> 8) * (b[2] >> 8);
}

/* 0x0800BDB0 */
s32 FUN_0800bdb0(const s32 *a, s32 ra, const s32 *b, s32 rb)
{
    s32 reach, x, y, z;
    s32 delta;

    reach = ra + rb;

    delta = a[0] - b[0];
    if (delta < 0) delta = -delta;
    x = delta - reach;

    delta = a[1] - b[1];
    if (delta < 0) delta = -delta;
    y = delta - reach;

    /* Ucuncu bilesen dogrudan hedefte; bkz. yukaridaki SIRA KANITI. */
    z = a[2] - b[2];
    if (z < 0) z = -z;
    z = z - reach;

    if (x < 0 && y < 0 && z < 0) return 1;
    return 0;
}
