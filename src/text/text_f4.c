/* Initialize and clear the text layer — 0x08064614-0x08064697
 *
 * Set the text context, DMA3-clear destination VRAM and copy the font palette
 * to palette RAM. One linear path without branches.
 *
 * ROM MEASUREMENTS:
 * 1. Destination: lsls r5,r5,#5 + (0xC0 << 19) = VRAM_BASE + tile*32.
 *    32 bytes is one 4bpp tile, so parameter one is a TILE INDEX. The ROM
 *    constructs 0x06000000 with movs #192 / lsls #19 rather than a pool load.
 *    VRAM_BASE reproduces this; no extern is needed (rule 1 does not apply
 *    to this ROM/VRAM constant; see COMPILER.md on shifted address constants).
 * 2. SetTextContext arguments: r0=dest, r1=28, r2=0x0884DE40, r3=0x088516C0,
 *    sp+0=(u8)(palette+bias), sp+4=1. Signature copied from set_text_context.c.
 * 3. Cross-check: stride 28 tiles *64 bytes =1792 bytes =0x380 halfwords,
 *    exactly the clear DMA count. Second DMA: 0x10 halfwords =16 colors,
 *    one 4bpp palette. Destination 0x05000000+palette*2 proves parameter two
 *    is a PALETTE ENTRY INDEX measured in colors.
 * 4. adds r2,r6,r2 + lsls/lsrs #24: fontIndex is palette+bias truncated to
 *    u8. The caller truncates because the prototype parameter is u8.
 *
 * This function MATCHED ON THE FIRST ATTEMPT. The following removal tests
 * were measured AFTER matching to establish which choices matter:
 * - Change prototype fontIndex u8 to u32: 107 differing bytes. The caller's
 *   lsls #24 / lsrs #24 disappears and size drops to 128 (caller side of
 *   rule 15); truncation inside the callee does not substitute for this.
 * - Remove volatile from stack fill: 61 bytes (rule 3); address formation
 *   is reordered relative to the constant load.
 * - Move fill=0 after src assignment: 8 bytes. ROM strh r3,[r0] precedes
 *   str r0,[r1,#0] (rule 19).
 * - Assign the bare dead REG_DMA3.control read to ime: 85 bytes, size 136.
 *   clear_text_area.c requires the opposite form; measure both rather than
 *   assuming the same family needs the same expression.
 * - Use separate volatile DmaChannel *dma instead of REG_DMA3: 17 bytes;
 *   the pool load moves earlier, as in menu_graphics.c.
 *
 * NO DIFFERENCE (all zero): explicit (u8)(palette+bias) since the prototype
 * already converts; s32 parameters since only shifts/addition are used;
 * remove dest and duplicate its expression (CSE merges); separate ime2 for
 * block two (nonoverlapping lifetimes reuse the register); write the palette
 * destination as (u16 *)PALETTE_BASE + palette.
 *
 * NAMING: tile index, palette index and font bias follow ROM usage.
 * GLYPH_TILES/GLYPH_WIDTHS follow SetTextContext parameters three/four in
 * the sibling file, where gGlyphTiles is marked PROVISIONAL. The function
 * name remains InitTextTilesAndPalette.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/text_f4.c  -> BYTE-MATCHING 132/132
 */

#include "gba_io.h"

#define TEXT_TILE_COUNT  28              /* 28 tiles * 64 bytes = 0x380 halfwords */
#define GLYPH_TILES      ((u8 *)0x0884DE40)
#define GLYPH_WIDTHS     ((u8 *)0x088516C0)
#define TEXT_PALETTE     ((const void *)0x0884DC40)
#define PALETTE_BASE     0x05000000
#define DMA_CLEAR_TEXT   0x81000380      /* fixed source, 16-bit, count 0x380 */
#define DMA_COPY_PALETTE 0x80000010      /* 16 colors */

extern void SetTextContext(u8 *vram, u32 stride, u8 *tiles, u8 *widths,
                           u8 fontIndex, u32 halfSpacing);

/* 0x08064614 */
void InitTextTilesAndPalette(u32 tile, u32 palette, u32 bias)
{
    volatile u16 fill;
    u16 ime;
    u8 *dest;

    dest = (u8 *)(VRAM_BASE + (tile << 5));

    SetTextContext(dest, TEXT_TILE_COUNT, GLYPH_TILES, GLYPH_WIDTHS,
                   palette + bias, 1);

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = dest;
    REG_DMA3.control = DMA_CLEAR_TEXT;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = TEXT_PALETTE;
    REG_DMA3.dst = (void *)(PALETTE_BASE + (palette << 1));
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;
}
