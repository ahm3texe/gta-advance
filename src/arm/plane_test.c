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
 * DURUM: PARK — 230/244 fark, ROM 196 bayt (bizimki 48 bayt UZUN).
 *
 * Bu, projedeki ILK ARM kipi denemesi.  Onceki tum denemeler teknik olarak
 * imkansizdi: derleme zinciri yalnizca Thumb'a bagliydi.
 *
 * DENENDI: -fomit-frame-pointer ARM bayraklarina eklendi (agbcc_arm
 * varsayilan olarak APCS cercevesi kuruyordu).  Kazanc VAR ama kucuk:
 * 244 -> 240 bayt, fark 230 -> 216.  Cerceve tek sebep degilmis; bayrak
 * yine de kalici olarak eklendi cunku ROM duz push kullaniyor.
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
 * ELENEN: yapi atamasiyla `ldm`/`stmia` blok transferi uretme denendi
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
