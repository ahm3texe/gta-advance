/* Sinir kutusunu daraltma — 0x0800A980-0x0800A9E3
 *
 * Iki gecis: once alt siniri kutunun (merkez - pay) degerine YUKSELTIYOR,
 * sonra ust siniri (merkez + pay) degerine INDIRIYOR. Yani gClipBounds,
 * verilen kutuyla kesistiriliyor.
 *
 * Ucuncu bilesen kenar payi yerine sabit 0x80000 kullaniyor ve ROM'da
 * ONEMLI BIR ASIMETRI var:
 *   birinci gecis: `ldr r5, [pc]` ile 0xFFF80000 (negatif, HAVUZDAN)
 *   ikinci gecis:  `movs r2,#128 / lsls r2,#12` ile 0x80000 (KAYDIRMAYLA)
 * Ikisi de TOPLAMA olarak yazilmali. `- 0x80000` yazmak sabiti kaydirmayla
 * kurdurup havuz kelimesini goturuyordu (96 bayt, ROM 100); 0xFFF80000
 * kaydirmayla uretilemedigi icin toplama bicimi havuzu zorunlu kiliyor.
 *
 * Taban (gClipBounds) ve pay, ilk karsilastirmadan ONCE kuruluyor
 * (kural 37) -- ROM `ldr r3` ve `lsls r1` ile ikisini de basta hazirliyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * HENUZ ESLESMIYOR: bizim cikti 96 bayt, ROM 100. Engel TEK bir yapisal
 * nedene iniyor: BIR FAZLA callee-saved register kullaniyoruz.
 *     ROM  : push {r4,r5,lr}       -- her bileseni r0'a TAZEDEN yukluyor
 *     bizim: push {r4,r5,r6,lr}    -- yuklenen degerler r5/r6'da yasiyor
 * Farklarin tamami bunun turevi (+0x14 ldr r5 vs r0, +0x20 ldr r6 vs r0,
 * +0x22/+0x24 sabit ve toplama register'lari).
 *
 * Yani ROM'da her `box->` okumasi kisa omurlu bir gecici; bizde derleyici
 * onlari ortak alt ifade olarak tutup omurlerini uzatiyor.
 *
 * Denenenler: `- 0x80000` yerine `+ (s32)0xFFF80000` (havuz yuklemesini
 * dogru uretti ama register sayisini degistirmedi); negatif sabiti ayri
 * bir `zlo` yereline almak (etkisiz). Ikisi de 96 bayt.
 *
 * Sonraki fikir: okumalarin omrunu kisaltmak icin her bileseni ayri bir
 * blok icinde ele almak, ya da pass 1 / pass 2'yi ayri yardimci
 * fonksiyonlara bolup inline ettirmek.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/clip_bounds.c
 */

#include "gba_types.h"

#define Z_LOWER  ((s32)0xFFF80000)   /* havuzdan yuklenir */
#define Z_UPPER  (0x80 << 12)          /* kaydirmayla kurulur */

typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

extern Vec3 gClipBounds;

/* 0x0800A980 */
void ClipBounds(Vec3 *box, u32 margin)
{
    Vec3 *bounds;
    s32 pad;

    bounds = &gClipBounds;
    pad = margin << 16;

    { s32 cand = box->x - pad;
      if (bounds->x < cand) bounds->x = cand; }
    { s32 cand = box->y - pad;
      if (bounds->y < cand) bounds->y = cand; }
    { s32 cand = box->z + Z_LOWER;
      if (bounds->z < cand) bounds->z = cand; }

    { s32 cand = ((volatile Vec3 *)box)->x + pad;
      if (bounds->x > cand) bounds->x = cand; }
    { s32 cand = ((volatile Vec3 *)box)->y + pad;
      if (bounds->y > cand) bounds->y = cand; }
    { s32 cand = ((volatile Vec3 *)box)->z + Z_UPPER;
      if (bounds->z > cand) bounds->z = cand; }
}
