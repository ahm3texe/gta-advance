/* Ucgen donusumu ve gorunurluk sinamasi — 0x0806A77C-0x0806A83F (196 bayt)
 *
 * KIP: ARM   <- derleme zinciri bu isaretle agbcc_arm'a geciyor
 *
 * Uc kosenin her birini (uc adet s16) okuyup taban ucluye gore donusturuyor,
 * donusmus ucluleri cikis tamponuna yaziyor, ilk iki kosenin CAPRAZ
 * CARPIMINI alip ucuncu koseyle NOKTA CARPIMINA sokuyor ve sonucu donduruyor.
 *
 * Donusum bilesenlere gore ASIMETRIK (ROM'dan olculdu):
 *     x = giris.x - taban.x     (rsb)
 *     y = taban.y + giris.y     (add)
 *     z = taban.z - giris.z     (sub)
 *
 * `ldrh rN,[r0],#2` + `lsl #16` + `asr #16`: s16 okuyup isaret genisletme.
 * Cikis isaretcisi her ucluden sonra 8 bayt atliyor (stmia + add #8), yani
 * hedef yapi 20 baytlik adimlarla ilerliyor.
 *
 * Fonksiyon tamamen ACILMIS; dongu yok, o yuzden kaynak da acilmis yazildi.
 *
 * DURUM: PARK — 171/196 fark, cikti 188 bayt (ROM 196).
 *
 * Bu, projedeki ILK ARM kipi denemesi.  Onceki tum denemeler teknik olarak
 * imkansizdi: derleme zinciri yalnizca Thumb'a bagliydi.
 *
 * BAYRAK KESFI (kalici olarak agbcc_build.py'ye eklendi):
 *   -fomit-frame-pointer                244 -> 240 bayt
 *   + -fno-schedule-insns               240 -> 216
 *   + -fno-schedule-insns2              216 -> 188  (ROM 196)
 * Bunlar olmadan agbcc_arm yigin cercevesi kurup ara sonuclari
 * tasiriyordu.  Butun ARM adaylari bundan yararlanacak.
 *
 * DAGITIM TAVANI — ONEMLI SINIR:
 * ROM ONBIR yazmac itiyor (r3,r4,...,sl,fp,ip,lr) ve hic tasma yapmiyor.
 * agbcc_arm on canli degerle sinandi: HER ZAMAN yalnizca sekiz yazmac
 * ({r4,r5,r6,r7,r8,r9,sl,lr}) itiyor ve fp/ip'yi genel dagitima HIC
 * sokmuyor.  Dokuz ayri bayrak denendi (-mapcs-frame, -mno-apcs-frame,
 * -ffixed-fp, -mapcs-reentrant, -fcall-used-fp, -fcall-used-ip, -O3,
 * -fforce-mem, -fno-schedule-insns); yalnizca sonuncusu fark yaratti,
 * hicbiri yazmac kumesini genisletmedi.
 *
 * KRITIK BULGU — ARM BOLGESI C'DEN ULASILABILIR:
 * ROM'daki ARM kodu barrel-shifter kaynasmalari kullaniyor
 * (`rsb r6, r3, r6, asr #16` gibi) ve bunlarin elle yazilmis assembly
 * olabilecegi supheniyle sinandi.  agbcc_arm bu kaliplarin UCUNU DE
 * C'den uretiyor (olculdu):
 *     a - (b >> 16)   ->  sub r0, r0, r1, asr #16
 *     (b >> 16) - a   ->  rsb r0, r0, r1, asr #16
 *     a + (b >> 16)   ->  add r0, r0, r1, asr #16
 * Yani 14920 baytlik ARM bolgesi normal bir eslestirme problemi.
 *
 * ELENEN (2): acik kaydirma bicimi -- kaynagi u16 okuyup `<< 16`'yi ayri
 * yerelde tutup `>> 16`'yi aritmetige kaynastirma denendi (ilerleyen ve
 * indeksli cikis yazimiyla).  IKISI DE GERILEDI: 232/207 ve 216/179.
 * Duz s16 + indeksli yazim (188/171) en iyisi olarak kaldi.
 *
 * ELENEN (1): yapi atamasiyla `ldm`/`stmia` blok transferi uretme denendi
 * (uc bicim: dizi indeksli, ilerleyen kaynak, ilerleyen kaynak+hedef).
 * UCU DE BELIRGIN GERILEDI: 304 bayt / ~288 fark.  agbcc_arm yapi
 * atamalarini daha AZ degil daha COK koda aciyor.  Skaler bicim
 * (240 bayt / 216 fark) en iyisi olarak kaldi.
 *
 * Sonraki adimlar (denenmedi):
 *   1. Cikis ofsetleri 20 bayt adimla ilerliyor; dst[5]/dst[10] yerine
 *      20 baytlik yapi dizisi denenebilir
 *   2. Capraz carpim terim sirasi ROM'daki mul/mla sirasiyla eslenmeli
 *      (ROM sonda `mul` + iki `mla` kullaniyor, bizimki ayri carpimlar)
 *   3. Kosе okumalari ROM'da post-index (`ldrh rN,[r0],#2`); kaynakta
 *      ilerleyen isaretci kullanmak bunu uretebilir
 *
 * Derleyici: agbcc_arm -mthumb-interwork -O2   (-fhex-asm ARM'da YOK)
 * Dogrulama:  make c-match FILE=src/arm/plane_test.c
 */

#include "gba_types.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* 0x0806A77C */
s32 FUN_0806a77c(const s16 *src, s32 *dst, const s32 *base)
{
    s32 bx, by, bz;
    s32 ax, ay, az;
    s32 nx, ny, nz;
    s32 cx, cy, cz;

    bx = base[0];
    by = base[1];
    bz = base[2];

    ax = src[0] - bx;
    ay = by + src[1];
    az = bz - src[2];
    dst[0] = ax;
    dst[1] = ay;
    dst[2] = az;

    nx = src[3] - bx;
    ny = by + src[4];
    nz = bz - src[5];
    dst[5] = nx;
    dst[6] = ny;
    dst[7] = nz;

    cx = az * ny - ay * nz;
    cy = ax * nz - az * nx;
    cz = ay * nx - ax * ny;

    ax = src[6] - bx;
    ay = by + src[7];
    az = bz - src[8];
    dst[10] = ax;
    dst[11] = ay;
    dst[12] = az;

    return cx * ax + cy * ay + cz * az;
}
