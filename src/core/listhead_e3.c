/* Dugumun kamera kutusuna uzakligindan puan uretme — 0x0800D450-0x0800D52F
 *
 * Ne yapiyor: dugumun ekrandaki dikdortgeni ile kamera noktasi arasindaki
 * KARESEL uzakligi hesaplayip yariya bolerek 0..0xFFFF araligina kirpiyor.
 * Sonucu kardes fonksiyon SortListByKey (src/core/listhead_e1.c) siralama
 * anahtarinin ust yarisi olarak kullaniyor; dugumun +0x1C alanina yaziyor.
 *
 * ROM'dan OKUNAN AYRINTILAR
 *
 * Kutu, dugumun kendi konumu ile sahip kaydinin (+0x10) dort ofsetinden
 * kuruluyor:
 *     x1 = node->x + owner->left      (owner +0x00)
 *     y1 = node->y + owner->top       (owner +0x04)
 *     x2 = node->x + owner->right     (owner +0x0C)
 *     y2 = node->y + owner->bottom    (owner +0x10)
 * node->x ve node->y `ldrsh` ile okunuyor, yani ISARETLI 16 bit (+0x06 ve
 * +0x0A). Thumb'da ldrsh yalnizca register-ofset biciminde oldugu icin ROM
 * her okumanin onune `movs rN,#6` / `movs rN,#10` koyuyor -- bu fazladan
 * komut derleyicinin degil komut kumesinin sonucu, kaynakta karsiligi yok.
 *
 * Kamera noktasi gClipBounds'un (0x03000014) ilk iki 16.16 degerinin TAM
 * KISMI: `ldrsh r1,[r0,#2]` ve `ldrsh r2,[r0,#6]`. Yani okunan sey
 * gClipBounds.x'in ust yarisi. Bu, ram_map.csv'deki "uc adet 16.16 deger"
 * kaydiyla birebir tutuyor. Tur olarak clip_bounds.c'deki `Vec3` AYNEN
 * kullanildi (extern tur tutarliligi denetleniyor).
 *
 * Uzaklik nokta-dikdortgen bicimi: eksen basina fark yalnizca nokta kutunun
 * DISINDA ise sayiliyor, icerideyse o eksenin katkisi sifir. ROM dokuz yaprak
 * blogu ayri ayri yaziyor (px<x1 / px>x2 / arada) x (py<y1 / py>y2 / arada).
 * Karsilastirmalar isaretli (`bge`/`ble`), tum ara degerler `int`.
 *
 * Sondaki kirpma: dist += 2; sahip kaydinin +0x08 alani (yariCap) POZITIFSE
 * dist'ten karesi cikariliyor ve negatife dusesse sifirlaniyor; sonra
 * `asrs #1` (isaretli yarilama) ve 0xFFFF tavani.
 *
 * OLCULEN UC AYRINTI (hepsi tek komutluk fark yaratti)
 *
 * 1) `dist += 2` ILE yariCap okumasinin SIRASI. Kaynakta once `dist += 2`
 *    yazildiginda `adds r1,#2` iki `ldr`den ONCE cikiyor; ROM onu tam
 *    aralarinda tutuyor. Okumayi one almak 8 baytlik (tek komut kaymasi)
 *    farki sifirladi: 216/224 -> 224/224.
 *
 * 2) YARICAP ICIN `box` YERELI TEKRAR KULLANILAMAZ. `radius = box->radius;`
 *    yazmak 220 bayt uretiyor: `box` if agacinin oteki ucuna kadar canli
 *    kaliyor ve sondaki `mov r4,ip` + `ldr r0,[r4,#16]` ciftini yok ediyor.
 *    ROM sahip isaretcisini ORADA YENIDEN YUKLUYOR, yani kaynakta tam yol
 *    (`node->owner->radius`) yazili. Blok sinirini gectigi icin agbcc'nin
 *    CSE'si de birlestirmiyor.
 *
 * 3) FARKLAR YAPRAK ICINDE YAZILIR, YAPRAKLARIN ONUNE ALINMAZ.
 *    `int dx = px - x1;` diye disariya cikarmak yine 220 bayt: ortak
 *    `subs r0,r1,r6` tek kopyaya iniyor. ROM ucunu de ayri ayri hesapliyor.
 *    Yapraklari acik yazinca blok ici CSE `subs` + `adds rX,r0,#0` + `muls`
 *    uclusunu zaten ROM'daki gibi uretiyor, kuyruk birlestirme (cross-jump)
 *    de 0x0800D4D4'teki paylasilan kuyrugu kendiliginden cikariyor.
 *
 * DENENIP ELENEN / ESDEGER BULUNAN YAZIMLAR
 *   - `radius = box->radius`            -> 220 bayt, ELENDI (bkz. 2)
 *   - yapraklarin onunde `dx` yereli    -> 220 bayt, ELENDI (bkz. 3)
 *   - `dist += 2` yariCap okumasindan once -> 216 bayt, ELENDI (bkz. 1)
 *   - kamera okumasini `s16 *view = (s16 *)&gClipBounds; px = view[1];`
 *     bicimiyle yazmak DA birebir esliyor. Isaretci kaligi yerine
 *     `(s16)(gClipBounds.x >> 16)` secildi: ayni baytlari uretiyor ama
 *     16.16 anlamini gizlemiyor. agbcc kaydirma + daraltmayi zaten tek
 *     `ldrsh`e katliyor.
 *
 * ESLESME: 224/224 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/listhead_e3.c
 */

#include "gba_types.h"

/* Puanin ust siniri: siralama anahtarinin ust yarisina sigmali. */
#define SCORE_MAX 0xFFFF

/* Dugumun +0x10'da gosterdigi kayit. Bu fonksiyon ilk bes kelimeyi
 * okuyor; +0x14'teki sira alani kardes dosyada kullaniliyor. */
typedef struct Owner {
    s32 left;                   /* +0x00 */
    s32 top;                    /* +0x04 */
    s32 radius;                 /* +0x08 */
    s32 right;                  /* +0x0C */
    s32 bottom;                 /* +0x10 */
} Owner;

typedef struct Node {
    u8  pad00[6];
    s16 x;                      /* +0x06 */
    u8  pad08[2];
    s16 y;                      /* +0x0A */
    u8  pad0c[4];
    Owner *owner;               /* +0x10 */
} Node;

/* 16.16 sinir kutusu (0x03000014); ClipBounds daraltiyor.
 * Tur src/core/clip_bounds.c ile birebir ayni olmali. */
typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

extern Vec3 gClipBounds;

/* 0x0800D450 */
int GetNodeBoxDistance(Node *node)
{
    Owner *box;
    int x, y;
    int x1, y1, x2, y2;
    int px, py;
    int dist;
    int radius;

    x = node->x;
    box = node->owner;
    x1 = x + box->left;
    y = node->y;
    y1 = y + box->top;
    x2 = x + box->right;
    y2 = y + box->bottom;

    /* 16.16 kamera konumunun tam kismi. */
    px = (s16)(gClipBounds.x >> 16);
    py = (s16)(gClipBounds.y >> 16);

    /* Nokta-dikdortgen karesel uzaklik; iceride kalan eksen katki vermez.
     * Farklar bilerek her yaprakta ayri yazildi (bkz. baslik, madde 3). */
    if (px < x1) {
        if (py < y1)
            dist = (px - x1) * (px - x1) + (py - y1) * (py - y1);
        else if (py > y2)
            dist = (px - x1) * (px - x1) + (py - y2) * (py - y2);
        else
            dist = (px - x1) * (px - x1);
    } else if (px > x2) {
        if (py < y1)
            dist = (px - x2) * (px - x2) + (py - y1) * (py - y1);
        else if (py > y2)
            dist = (px - x2) * (px - x2) + (py - y2) * (py - y2);
        else
            dist = (px - x2) * (px - x2);
    } else {
        if (py < y1)
            dist = (py - y1) * (py - y1);
        else if (py > y2)
            dist = (py - y2) * (py - y2);
        else
            dist = 0;
    }

    /* Sahip isaretcisi burada YENIDEN okunuyor (bkz. baslik, madde 2). */
    radius = node->owner->radius;
    dist += 2;
    if (radius > 0) {
        dist -= radius * radius;
        if (dist < 0)
            dist = 0;
    }

    dist >>= 1;
    if (dist > SCORE_MAX)
        dist = SCORE_MAX;
    return dist;
}
