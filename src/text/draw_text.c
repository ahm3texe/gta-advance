/* Metin cizme — 0x0806434C-0x0806458F
 *
 * Uc cizim varyanti (sol, ortali, saga hizali) ile glif ve dize genislik
 * hesabi. Metinde '@' + '0'/'8'/'9' bir RENK KACISI olarak atlanir; oyun
 * metnindeki "@8AMMU-NATION@0" kalibi budur.
 *
 * Cizimden once her karakter newlib _toupper'dan geciriliyor; ROM'daki tum
 * oyun metninin buyuk harf olmasinin sebebi budur.
 *
 * GetTextWidth, GetGlyphWidth'i cagirmaz; ayni hesabi kendi icinde
 * tekrarlar. ROM'da da iki ayri kod kopyasi var.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/draw_text.c
 */

#include "gba_io.h"

#define GLYPH_FIRST      32
#define GLYPH_SUBSTITUTE 146
#define GLYPH_APOSTROPHE 39
#define COLOUR_ESCAPE    64
#define SCREEN_RIGHT     239
#define SCREEN_BOTTOM    159
#define LINE_HEIGHT      16
#define LINE_HEIGHT_HALF 8
#define TILE_SHIFT       3

extern u32 gTextRowStride;
extern u8 *gTextVramBase;
extern u8 *gGlyphWidths;
extern u32 gFontIndex;
extern u32 gHalfLineSpacing;

extern u8   _toupper(u8 ch);
extern void PlaceGlyph(u8 ch, s32 x, s32 y);

s32 GetGlyphWidth(u32 ch);
s32 GetTextWidth(const u8 *text);

#define IS_COLOUR_DIGIT(c) ((u8)((c) - 56) <= 1 || (c) == 48)

static __inline__ s32 GlyphAdvance(u32 ch)
{
    s32 index;
    u32 width;
    s32 advance;

    index = ch;
    if (index == GLYPH_SUBSTITUTE)
        index = GLYPH_APOSTROPHE;

    index = _toupper((u8)index);
    index -= GLYPH_FIRST;
    if (index < 0)
        return 0;

    width = gGlyphWidths[index];
    advance = width + 2;
    if (advance & 1)
        advance = width + 3;

    return advance;
}

static __inline__ s32 DrawTextAt(const u8 *text, s32 x, s32 y)
{
    s32 left;
    u8 ch;

    left = x;

    while ((ch = *text++) != 0) {
        if (ch == COLOUR_ESCAPE && IS_COLOUR_DIGIT(*text)) {
            text++;
            continue;
        }

        if (ch == 10 || ch == 13) {
            x = left;
            if (gHalfLineSpacing == 0)
                y += LINE_HEIGHT;
            else
                y += LINE_HEIGHT_HALF;
            continue;
        }

        ch = _toupper(ch);
        PlaceGlyph(ch, x, y);
        x += GetGlyphWidth(ch);
        if (x > SCREEN_RIGHT)
            break;
    }

    return x;
}

/* 0x0806434C */
void SetFontIndex(u32 index)
{
    u32 value;

    value = (u8)index;
    gFontIndex = value;
}

/* 0x0806435C */
s32 DrawText(const u8 *text, s32 x, s32 y)
{
    return DrawTextAt(text, x, y);
}

/* 0x080643D8 */
void DrawTextCentred(const u8 *text, s32 x, s32 y)
{
    s32 width;

    width = GetTextWidth(text) >> 1;
    DrawTextAt(text, x - width, y);
}

/* 0x08064460 */
void DrawTextRightAligned(const u8 *text, s32 x, s32 y)
{
    s32 width;

    width = GetTextWidth(text);
    DrawTextAt(text, x - width, y);
}

/* 0x080644E4 */
s32 GetGlyphWidth(u32 ch)
{
    return GlyphAdvance(ch);
}

/* 0x08064524 */
s32 GetTextWidth(const u8 *text)
{
    s32 total;
    u8 ch;

    total = 0;

    while ((ch = *text++) != 0) {
        if (ch == COLOUR_ESCAPE && IS_COLOUR_DIGIT(*text)) {
            text++;
            continue;
        }

        total += GlyphAdvance(ch);
    }

    return total;
}
