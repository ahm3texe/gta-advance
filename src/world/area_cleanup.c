/* Alan temizligi — 0x08030CB4-0x08030D0B
 *
 * Bekleyen temizlik bayragi kuruluysa iki VRAM konumuna ucer yarim soz
 * dolduruyor, bir blogu serbest birakiyor ve bayragi temizliyor.
 *
 * HENUZ ESLESMIYOR: 7 bayt fark (onceki durum 56 idi).
 *
 * IKI ENGEL COZULDU:
 *   1. Ayri yasam araligi (kural 37). ROM dolguyu r1'e yukleyip
 *      `adds r0, r1, #0` ile r0'a kopyaliyordu; tek `fill` degiskeni bu
 *      kopyayi uretemez. `fill2 = fill` ekleyip iki store'u ayirmak
 *      56 -> 17 yaptı. Kopyanin yonu ve ikisini de sabitten kurmak
 *      farketmiyor (uc bicim de 17): agbcc kopyayi ayni ele aliyor.
 *   2. Havuz sirasi. ROM havuza once blok adresini, sonra dolguyu
 *      koyuyor. `block` atamasini `fill`in ONUNE almak 17 -> 7 yaptı.
 *      Havuz sirasi, sabitlerin KAYNAKTA ilk referans sirasini izliyor.
 *
 * KALAN 7 BAYT tek bir yapisal nedene iniyor: ROM bes register'a sigiyor
 * (`push {r4,lr}`), biz alti istiyoruz (`push {r4,r5,lr}`). Farklarin
 * tamami bunun turevi -- ROM block'u tek callee-saved r4'te tutup iki
 * dolguyu scratch r0/r1'de birakiyor; bizde fill2 r4'u kapiyor ve block
 * r5'e itiliyor. Yani ROM'da dongu boyunca yasayan bir deger daha az;
 * muhtemelen `i` sayaci ayri bir register tutmuyor.
 * Bildirim sirasi bu fonksiyonda ETKISIZ (uc permutasyon da 7 verdi) --
 * ClearTextArea'nin aksine; oradaki hassasiyet genellenebilir degil.
 *
 * PERMUTER SONUCU (8.871 yineleme): bizim olcumumuzde KAZANC YOK.
 * Arac "yeni en iyi skor 45 (50 yerine)" dedi ama uc adayin da bayt farki
 * 7'de kaldi; yalnizca farkin yeri oynadi (0x1A -> 0x1C). Permuter'in
 * skoru komut agirlikli bir sezgisel, birebir bayt esitligi degil --
 * ara skorlara guvenilmez, anlamli olan tek deger 0.
 * Bulgusu yine de bilgi verdi: `i = 0` yerine ayri bir yerelden
 * (`start = 0; i = start`) gecmek fark SAYISINI degistirmeden YERINI
 * oynatiyor, yani sayacin yasam araligi dagitimi gercekten etkiliyor.
 * Kok neden (bir fazla canli deger) degismedi.
 *
 * Onceki turda elenenler (fill2 YOKKEN olculmustu, artik gecersiz sayilmali):
 * blogu dongu oncesi yuklemek 61, dolguyu int yapmak 61, satir ici 63.
 *
 * Ayni kumedeki eslesen uc fonksiyon: src/world/area_flags.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/area_cleanup.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define TILE_FILL      0xF0E8
#define TILE_RUN       3
#define TILE_ROW_LEFT   ((u16 *)0x06009818)
#define TILE_ROW_RIGHT  ((u16 *)0x06009858)

typedef struct Progress {
    u8 pad0000[0x137E];
    u8 pendingCleanup;          /* +0x137E */
} Progress;

extern u32      gRam02026E80;

extern void FUN_08013abc(u32 *block);

/* 0x08030CB4 */
void CleanupAreaTiles(void)
{
    u16 *left;
    u16 *right;
    u32 *block;
    u32 i;
    u16 fill;
    u16 fill2;

    if (((Progress *)gRam02025810)->pendingCleanup == 0)
        return;

    i = 0;
    block = &gRam02026E80;
    fill = TILE_FILL;
    fill2 = fill;
    right = TILE_ROW_RIGHT;
    left = TILE_ROW_LEFT;
    do {
        *left = fill;
        *right = fill2;
        right++;
        left++;
        i++;
    } while (i <= TILE_RUN - 1);

    FUN_08013abc(block);
    ((Progress *)gRam02025810)->pendingCleanup = 0;
}
