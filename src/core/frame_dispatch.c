/* Sekiz koseli hacimden alti yuzey duzlemi kurma — 0x0800AB88-0x0800AF47
 *
 * Girdi 8 x 12 baytlik (s32 x,y,z) kose dizisi; degerler 8.8 sabit noktada,
 * her bilesen once `>> 8` ile tam sayiya indiriliyor (ROM'da `asrs #8`).
 * Cikti yapisinin +0x00 alanina duzlem sayisi (6) yaziliyor, +0x04'ten
 * baslayan alti adet 16 baytlik duzlem (x,y,z,d) dolduruluyor.
 *
 * Her yuzey ayni kalibi tekrarliyor — kaynakta ALTI KEZ ACIK yazili, ROM'da
 * dongu ve dallanma YOK (CFG tek temel blok):
 *     a = P1 - P0,  b = P2 - P0
 *     n = a x b                            (capraz carpim)
 *     n.d = -n.x*P0.x - n.y*P0.y - n.z*P0.z
 *     FUN_0800b0d4(&n, &out->scaled[i]);   normali olcekleyip yaziyor
 *     out->planes[i] = n;                  16 baytlik yapi atamasi
 *
 * Yuzeylerin kose ucluleri (kaynak sirasiyla):
 *     0: (P0, P1, P5)   1: (P2, P3, P7)   2: (P3, P0, P4)
 *     3: (P1, P2, P6)   4: (P2, P1, P0)   5: (P6, P7, P4)
 *
 * IKI ORIJINAL TUHAFLIK — ikisi de ROM'dan OLCULDU, uydurma degil:
 *
 *  1) FUN_0800b0d4 (0x0800B0D4) ikinci argumanina UC kelime yaziyor
 *     (`str r0,[r5,#0]` / `[r5,#4]` / `[r5,#8]`), buna karsilik cagrilar
 *     +0x64, +0x68, +0x6C, +0x70, +0x74, +0x78 adreslerini veriyor — yani
 *     DORT bayt aralikla ve yazimlar ust uste biniyor; son cagri +0x80'e
 *     kadar yaziyor. `scaled[]` bu yuzden 8 elemanli bildirildi.
 *
 *  2) Bes numarali yuzeyde (P2, P1, P0) z bilesenlerinin ATAMA HEDEFLERI
 *     yer degistirmis: ucuncu yukleme `z2`ye, dokuzuncu yukleme `z0`a
 *     gidiyor. Kanit yalniz yukleme sirasi degil, kullanim da — o blokta
 *         az = z1 - r8,  bz = ip - r8,  d bileseninde yine r8
 *     kaliplari ancak r8 = P0.z ve ip = P2.z ile ciktigi icin z0 = P0.z,
 *     z2 = P2.z olmus. Diger bes blokta z0 hep ilk kosenin z'si. Kalip
 *     degil, orijinaldeki yazim hatasi; korunmasi eslesme icin ZORUNLU.
 *
 * Yigin cercevesi 24 bayt: +0x00..+0x0F yerel `n`, +0x10 ve +0x14 iki
 * parametrenin dokumu. agbcc her iki parametreyi de yaziyor ve her blokta
 * geri okuyor (blok basina 3 kez: yedinci `asrs` taban yazmacini eziyor,
 * reload yeniden yukluyor).
 *
 * OLCULEN BELIRLEYICI AYRINTI — YERELLERIN KAPSAMI (1003/1082 fark -> 0):
 *   Ilk denemede dokuz skaler ve alti fark her yuzey icin AYRI `{ }`
 *   blogunda bildirilmisti (kural 40'in "kisa omurlu gecici" mantigi).
 *   Sonuc 1082 bayt: `pts` parametresi r5'e oturuyor, dolayisiyla bir
 *   yazmac eksik kaliyor ve her blokta `x0` yigina tasiyor — ustelik her
 *   blok kendi tasma yuvasini aldigi icin cerceve 44 bayta cikiyor.
 *   dump_alloc.py bunu dogruluyordu: `pts` refs=55 / omur=281 -> oncelik
 *   0.979 iken blok yerelleri 0.10-0.22 bandinda kaliyor, yani parametre
 *   HER ZAMAN once dagitiliyordu.
 *   Skalerleri FONKSIYON KAPSAMINDA tek takim olarak bildirmek omurlerini
 *   tum fonksiyona yayiyor; boylece onlar `pts`ten once dagitilip r4-r7'yi
 *   kapatiyor ve `pts` ile `out` tasiyor — ROM'un tam yerlesimi.
 *   Yani kural 40 ("her bileseni kendi blogunda tut") BU fonksiyonda
 *   TERSINE calisiyor: tekrarlanan buyuk govdelerde ortak yerel takimi,
 *   parametreyi yazmactan atmak icin gereken uzun omru saglayan sey.
 *   Denenip ELENEN diger iki yol: blok basina `p = pts;` kopyasi (agbcc
 *   kopyayi kaynastiriyor, cikti bayti bayta ayni: 1082) ve blok basina
 *   `o/u/v` kose isaretcileri (daha kotu: 1150).
 *
 * UYGULANAN DIGER KURALLAR:
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - Kural 14/19: tekrarlanan govde donguye sarilmadan acik yazildi;
 *     blok icindeki atama sirasi ROM'un yukleme sirasiyla birebir.
 *   - Kural 32: `out->planes[i] = n;` yapi atamasi `ldmia/stmia {r2,r3,r4}`
 *     + `ldr/str` ciftini uretiyor; dort kelimeyi elle kopyalamak uretmiyor.
 *   - Kural 2: `pts[i].x` ve `out->planes[i]` biciminde dogrudan uye
 *     erisimi; taban parametre oldugu icin havuz sabitine katlanma yok.
 *
 * ESLESME: 960/960 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/big_b2.c
 */

#include "gba_types.h"

/* Hacmin kosesi: 8.8 sabit noktada uc bilesen. */
typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* Duzlem: normal + sabit terim. */
typedef struct Plane {
    s32 x;
    s32 y;
    s32 z;
    s32 d;
} Plane;

#define PLANE_COUNT 6

/* Cikti hacmi. `scaled` alani FUN_0800b0d4'un ust uste binen yazimlarini
 * kapsayacak kadar genis tutuldu (dosya basindaki 1 numarali not). */
typedef struct Volume {
    s32   planeCount;              /* 0x00 */
    Plane planes[PLANE_COUNT];     /* 0x04 */
    s32   scaled[8];               /* 0x64 */
} Volume;

/* 0x0800B0D4 — normali uzunlugunun tersiyle olcekleyip uc kelime yaziyor. */
extern void FUN_0800b0d4(Plane *plane, s32 *out);

/* 0x0800AB88 */
void FUN_0800ab88(Vec3 *pts, Volume *out)
{
    Plane n;
    s32 x0, y0, z0, x1, y1, z1, x2, y2, z2;
    s32 ax, ay, az, bx, by, bz;

    out->planeCount = PLANE_COUNT;

    /* Yuzey 0: (P0, P1, P5) */
    x0 = pts[0].x >> 8;
    y0 = pts[0].y >> 8;
    z0 = pts[0].z >> 8;
    x1 = pts[1].x >> 8;
    y1 = pts[1].y >> 8;
    z1 = pts[1].z >> 8;
    x2 = pts[5].x >> 8;
    y2 = pts[5].y >> 8;
    z2 = pts[5].z >> 8;

    ax = x1 - x0;
    ay = y1 - y0;
    az = z1 - z0;
    bx = x2 - x0;
    by = y2 - y0;
    bz = z2 - z0;

    n.x = ay * bz - az * by;
    n.y = az * bx - ax * bz;
    n.z = ax * by - ay * bx;
    n.d = -n.x * x0 - n.y * y0 - n.z * z0;

    FUN_0800b0d4(&n, &out->scaled[0]);
    out->planes[0] = n;

    /* Yuzey 1: (P2, P3, P7) */
    x0 = pts[2].x >> 8;
    y0 = pts[2].y >> 8;
    z0 = pts[2].z >> 8;
    x1 = pts[3].x >> 8;
    y1 = pts[3].y >> 8;
    z1 = pts[3].z >> 8;
    x2 = pts[7].x >> 8;
    y2 = pts[7].y >> 8;
    z2 = pts[7].z >> 8;

    ax = x1 - x0;
    ay = y1 - y0;
    az = z1 - z0;
    bx = x2 - x0;
    by = y2 - y0;
    bz = z2 - z0;

    n.x = ay * bz - az * by;
    n.y = az * bx - ax * bz;
    n.z = ax * by - ay * bx;
    n.d = -n.x * x0 - n.y * y0 - n.z * z0;

    FUN_0800b0d4(&n, &out->scaled[1]);
    out->planes[1] = n;

    /* Yuzey 2: (P3, P0, P4) */
    x0 = pts[3].x >> 8;
    y0 = pts[3].y >> 8;
    z0 = pts[3].z >> 8;
    x1 = pts[0].x >> 8;
    y1 = pts[0].y >> 8;
    z1 = pts[0].z >> 8;
    x2 = pts[4].x >> 8;
    y2 = pts[4].y >> 8;
    z2 = pts[4].z >> 8;

    ax = x1 - x0;
    ay = y1 - y0;
    az = z1 - z0;
    bx = x2 - x0;
    by = y2 - y0;
    bz = z2 - z0;

    n.x = ay * bz - az * by;
    n.y = az * bx - ax * bz;
    n.z = ax * by - ay * bx;
    n.d = -n.x * x0 - n.y * y0 - n.z * z0;

    FUN_0800b0d4(&n, &out->scaled[2]);
    out->planes[2] = n;

    /* Yuzey 3: (P1, P2, P6) */
    x0 = pts[1].x >> 8;
    y0 = pts[1].y >> 8;
    z0 = pts[1].z >> 8;
    x1 = pts[2].x >> 8;
    y1 = pts[2].y >> 8;
    z1 = pts[2].z >> 8;
    x2 = pts[6].x >> 8;
    y2 = pts[6].y >> 8;
    z2 = pts[6].z >> 8;

    ax = x1 - x0;
    ay = y1 - y0;
    az = z1 - z0;
    bx = x2 - x0;
    by = y2 - y0;
    bz = z2 - z0;

    n.x = ay * bz - az * by;
    n.y = az * bx - ax * bz;
    n.z = ax * by - ay * bx;
    n.d = -n.x * x0 - n.y * y0 - n.z * z0;

    FUN_0800b0d4(&n, &out->scaled[3]);
    out->planes[3] = n;

    /* Yuzey 4: (P2, P1, P0) — z0 ve z2 ATAMALARI yer degistirmis;
     * bkz. dosya basindaki 2 numarali not. */
    x0 = pts[2].x >> 8;
    y0 = pts[2].y >> 8;
    z2 = pts[2].z >> 8;
    x1 = pts[1].x >> 8;
    y1 = pts[1].y >> 8;
    z1 = pts[1].z >> 8;
    x2 = pts[0].x >> 8;
    y2 = pts[0].y >> 8;
    z0 = pts[0].z >> 8;

    ax = x1 - x0;
    ay = y1 - y0;
    az = z1 - z0;
    bx = x2 - x0;
    by = y2 - y0;
    bz = z2 - z0;

    n.x = ay * bz - az * by;
    n.y = az * bx - ax * bz;
    n.z = ax * by - ay * bx;
    n.d = -n.x * x0 - n.y * y0 - n.z * z0;

    FUN_0800b0d4(&n, &out->scaled[4]);
    out->planes[4] = n;

    /* Yuzey 5: (P6, P7, P4) */
    x0 = pts[6].x >> 8;
    y0 = pts[6].y >> 8;
    z0 = pts[6].z >> 8;
    x1 = pts[7].x >> 8;
    y1 = pts[7].y >> 8;
    z1 = pts[7].z >> 8;
    x2 = pts[4].x >> 8;
    y2 = pts[4].y >> 8;
    z2 = pts[4].z >> 8;

    ax = x1 - x0;
    ay = y1 - y0;
    az = z1 - z0;
    bx = x2 - x0;
    by = y2 - y0;
    bz = z2 - z0;

    n.x = ay * bz - az * by;
    n.y = az * bx - ax * bz;
    n.z = ax * by - ay * bx;
    n.d = -n.x * x0 - n.y * y0 - n.z * z0;

    FUN_0800b0d4(&n, &out->scaled[5]);
    out->planes[5] = n;
}
