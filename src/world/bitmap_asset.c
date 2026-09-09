/* The frame and palette loader — 0x08065574-0x0806564F
 *
 * Takes one of the 20-byte records at 0x08FD17C8 and writes its palette and
 * frame data to the destination.  Both are transferred either by RL
 * decompression or by DMA3, according to a flag.  On the DMA paths IME is
 * turned off and restored, and the control register is read once and discarded
 * (so the write settles in hardware).
 *
 * The frame-count division is SIGNED: the ROM emits
 * `cmp #0 / bge / adds #3 / asrs #2`, so the source writes
 * `(s32)(height * width) / 4`, not `>> 2`.
 *
 * The final loop adds the palette offset to every half-word; the bound is
 * recomputed on each iteration, i.e. it is written in place in the `for`
 * condition.
 *
 * TWO MEASUREMENTS:
 *  - The IME save must be in TWO SEPARATE variables.  With a single variable
 *    the live range spans both blocks, `dest` is pushed into a high register
 *    and the extra push/pop adds 12 bytes.  The ROM keeps the second save in
 *    `ip` -- scratch that need not be saved.
 *  - The multiplication operand order: it must be written `width * height`.
 *    agbcc loads the second operand FIRST, so the order in the source is the
 *    reverse of the ROM's load order.  Written the other way it diverges by 4
 *    bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/bitmap_asset.c
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
