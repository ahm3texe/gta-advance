/* Set the text drawing context — 0x0806430C-0x0806434B
 *
 * Write six parameters to six globals without branches. Four arrive in
 * r0-r3, two at sp+16/sp+20 after the 16-byte push {r4,r5,r6,lr}. Parameter
 * five is truncated to u8 with lsls #24 / lsrs #24.
 * Rule 35: pop {r0}; bx r0 indicates void.
 *
 * NAMING — resolved conflict: callers menu_screen.c/menu_loop.c treated this
 * as a TILE LOADER (tilesA/tilesB), but disassembly shows a pure context setter.
 * Usage settled the issue: gGlyphWidths[index] is indexed as an array in two
 * MATCHING files (clear_text_area.c:115, draw_text.c:58), proving it is a
 * table pointer. The caller's tilesB name was a mistaken inference.
 *
 * OPEN QUESTION: gFontIndex receives 160 (MENU_TILE_WIDTH) here and
 * 160/192/128/240/242 in traces, resembling width/position more than a font
 * index. The name is QUESTIONABLE but retained without evidence for a better
 * one and to avoid disturbing matching files.
 *
 * NEW SYMBOL: 0x02036308 was unmapped. It receives 0x08831880, exactly 0xE000
 * bytes before gGlyphWidths' 0x0883F880, consistent with glyph bitmaps followed
 * by a width table. gGlyphTiles is therefore a PROVISIONAL name.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/set_text_context.c
 */

#include "gba_types.h"

extern u8  *gTextVramBase;      /* 0x02036310 */
extern u32  gTextRowStride;     /* 0x0203630C */
extern u8  *gGlyphTiles;        /* 0x02036308 — PROVISIONAL name */
extern u8  *gGlyphWidths;       /* 0x02036314 */
extern u32  gFontIndex;         /* 0x02036318 — questionable name; see above */
extern u32  gHalfLineSpacing;   /* 0x0203631C */

/* 0x0806430C */
void SetTextContext(u8 *vram, u32 stride, u8 *tiles, u8 *widths,
                    u8 fontIndex, u32 halfSpacing)
{
    gTextVramBase = vram;
    gTextRowStride = stride;
    gGlyphTiles = tiles;
    gGlyphWidths = widths;
    gFontIndex = fontIndex;
    gHalfLineSpacing = halfSpacing;
}
