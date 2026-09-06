/* Kare ve palet yukleyici — 0x08065574-0x0806564F
 *
 * 0x08FD17C8'deki 20 baytlik kayitlardan birini alip paleti ve kare
 * verisini hedefe yaziyor. Her ikisi de bayraga gore ya RL acilimiyla ya
 * da DMA3 ile aktariliyor. DMA yollarinda IME kapatilip geri yukleniyor
 * ve denetim yazmaci bir kez bos okunuyor (donanimda yazimin oturmasi
 * icin).
 *
 * Kare sayisi bolumu ISARETLI: ROM `cmp #0 / bge / adds #3 / asrs #2`
 * uretiyor, yani kaynak `(s32)(boy * en) / 4` yaziyor, `>> 2` degil.
 *
 * Son dongu palet ofsetini her yarim kelimeye ekliyor; sinir her
 * yinelemede yeniden hesaplaniyor, yani `for` kosulunda yerinde yazili.
 *
 * IKI OLCUM:
 *  - IME kaydi IKI AYRI degiskende olmali. Tek degisken kullanilinca
 *    canli aralik iki blogu birden kapsiyor, `dest` yuksek yazmaca
 *    itiliyor ve fazladan push/pop 12 bayt ekliyor. ROM ikinci kaydi
 *    `ip`'de tutuyor -- kaydedilmesi gerekmeyen scratch.
 *  - Carpim operand sirasi: `en * boy` yazilmali. agbcc ikinci operandi
 *    ONCE yukluyor, yani kaynaktaki sira ROM'un yukleme sirasinin
 *    tersidir. Ters yazilinca 4 bayt sapiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bitmap_asset.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define ASSET_MAX_INDEX  1
#define FLAG_TILES_RL    1
#define FLAG_PAL_RL      2
#define PALETTE_BASE     0x05000000
#define DMA_ENABLE       0x80000000
#define DMA_ENABLE_32    0x84000000

typedef struct BitmapAsset {
    const void *tiles;          /* +0x00 */
    const void *palette;        /* +0x04 */
    u32         flags;          /* +0x08 */
    u16         width;          /* +0x0C */
    u16         height;         /* +0x0E */
    u16         paletteWords;   /* +0x10 */
    u16         pad12;
} BitmapAsset;

extern const BitmapAsset gBitmapAssets[];
extern void RLUnCompVram(const void *src, void *dst);

/* 0x08065574 */
void LoadBitmapAsset(u32 index, u16 *dest, u32 palOffset)
{
    const BitmapAsset *asset;
    u16               *palDest;
    u16                savedPalIme;
    u16                savedTileIme;
    s32                j;

    if (index > ASSET_MAX_INDEX)
        return;

    asset   = &gBitmapAssets[index];
    palDest = (u16 *)(PALETTE_BASE + palOffset * 2);

    if ((asset->flags & FLAG_PAL_RL) != 0) {
        RLUnCompVram(asset->palette, palDest);
    } else {
        savedPalIme = REG_IME;
        REG_IME     = 0;
        REG_DMA3.src     = asset->palette;
        REG_DMA3.dst     = palDest;
        REG_DMA3.control = DMA_ENABLE | asset->paletteWords;
        REG_DMA3.control;
        REG_IME = savedPalIme;
    }

    if ((asset->flags & FLAG_TILES_RL) != 0) {
        RLUnCompVram(asset->tiles, dest);
    } else {
        savedTileIme = REG_IME;
        REG_IME      = 0;
        REG_DMA3.src     = asset->tiles;
        REG_DMA3.dst     = dest;
        REG_DMA3.control = DMA_ENABLE_32
                         | (u32)((s32)(asset->width * asset->height) / 4);
        REG_DMA3.control;
        REG_IME = savedTileIme;
    }

    if (palOffset != 0) {
        for (j = 0; j < (s32)((asset->width * asset->height) << 5); j += 2)
            *(u16 *)((u8 *)dest + j) += (palOffset << 8) | palOffset;
    }
}
