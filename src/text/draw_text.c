/* Metin cizme — 0x0806434C-0x08064613
 *
 * Uc cizim varyanti (sol, ortali, saga hizali), glif ve dize genislik
 * hesabi ve bir metin alani temizleyicisi.
 *
 * Metin icinde '@' + '0'/'8'/'9' bir renk kacisi olarak atlanir; oyun
 * metninde gecen "@8AMMU-NATION@0" kalibi budur.
 *
 * GetTextWidth, GetGlyphWidth'i CAGIRMAZ; ayni hesabi kendi icinde
 * tekrarlar. ROM'da da oyle: iki ayri kod kopyasi var.
 *
 * DURUM: 7 fonksiyonun 1'i (SetFontIndex) byte-matching. Yapi ve kontrol
 * akisi dogru cikarildi; kalan fark register dagitiminda.
 *
 * ANA TIKANIKLIK — GetGlyphWidth (0x080644E4, 18/64 bayt fark):
 *   ROM  : adds r1, r0, #0 / cmp r1, #146 / ... / lsls r0, r1, #24
 *   bizim:                   cmp r0, #146 / ... / lsls r0, r0, #24
 * ROM degeri ayri bir register'da (r1) tutuyor, bizimki parametreyi (r0)
 * yerinde kullaniyor. Yani kaynakta parametreden AYRI bir yerel degisken
 * var ve agbcc onu bizde birlestiriyor.
 * Denenip tutmayanlar: u8/u32 parametre, s32/u32 yerel, bildirim sirasi
 * permutasyonlari, index tipini degistirme.
 *
 * DrawText yalnizca 1 bayt farkli ve o da GetGlyphWidth'e giden `bl`
 * ofseti -- GetGlyphWidth tuttugu an duzelir. DrawTextCentred /
 * DrawTextRightAligned / GetTextWidth / ClearTextArea ayni koku paylasiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/draw_text.c
 */

#include "gba_io.h"

#define GLYPH_FIRST      32
#define GLYPH_SUBSTITUTE 146
#define GLYPH_APOSTROPHE 39
#define COLOUR_ESCAPE    64      /* '@' */
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

/* 0x0806DE84 newlib _toupper: metin cizilmeden once buyuk harfe cevriliyor.
 * ROM'daki tum oyun metninin buyuk harf olmasinin sebebi budur. */
extern u8   _toupper(u8 ch);
/* 0x08064020 henuz adlandirilmadi; govdesi incelenmedi. */
extern void FUN_08064020(u8 ch, s32 x, s32 y); /* glifi cizer  */

/* ROM sirasinda cizim fonksiyonlari once geliyor; ileri bildirimler sart. */
s32 GetGlyphWidth(u32 ch);
s32 GetTextWidth(const u8 *text);

/* '@' sonrasi renk basamagi mi */
#define IS_COLOUR_DIGIT(c) ((u8)((c) - 56) <= 1 || (c) == 48)

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
        FUN_08064020(ch, x, y);
        x += GetGlyphWidth(ch);
        if (x > SCREEN_RIGHT)
            break;
    }

    return x;
}

/* 0x080643D8 */
void DrawTextCentred(const u8 *text, s32 x, s32 y)
{
    s32 left;
    u8 ch;

    x -= GetTextWidth(text) >> 1;
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
        FUN_08064020(ch, x, y);
        x += GetGlyphWidth(ch);
        if (x > SCREEN_RIGHT)
            break;
    }
}

/* 0x08064460 */
void DrawTextRightAligned(const u8 *text, s32 x, s32 y)
{
    s32 left;
    u8 ch;

    x -= GetTextWidth(text);
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
        FUN_08064020(ch, x, y);
        x += GetGlyphWidth(ch);
        if (x > SCREEN_RIGHT)
            break;
    }
}

/* 0x080644E4 */
s32 GetGlyphWidth(u32 ch)
{
    u32 code;
    s32 index;
    u32 width;
    s32 advance;

    code = ch;
    if (code == GLYPH_SUBSTITUTE)
        code = GLYPH_APOSTROPHE;

    index = (u8)_toupper((u8)code) - GLYPH_FIRST;
    if (index < 0)
        return 0;

    width = gGlyphWidths[index];
    advance = width + 2;
    if ((width + 2) & 1)
        advance = width + 3;

    return advance;
}

/* 0x08064524 */
s32 GetTextWidth(const u8 *text)
{
    s32 total;
    u8 ch;
    u32 code;
    s32 index;
    u32 width;
    s32 advance;

    total = 0;

    while ((ch = *text++) != 0) {
        if (ch == COLOUR_ESCAPE && IS_COLOUR_DIGIT(*text)) {
            text++;
            continue;
        }

        code = ch;
        if (code == GLYPH_SUBSTITUTE)
            code = GLYPH_APOSTROPHE;

        index = (u8)_toupper((u8)code) - GLYPH_FIRST;
        if (index < 0) {
            advance = 0;
        } else {
            width = gGlyphWidths[index];
            advance = width + 2;
            if ((width + 2) & 1)
                advance = width + 3;
        }
        total += advance;
    }

    return total;
}

/* 0x08064590 */
void ClearTextArea(s32 x, s32 y, s32 height)
{
    volatile u16 fill;
    u16 ime;
    u8 *dest;
    u32 stride;
    u32 control;

    if ((u32)x > SCREEN_RIGHT)
        return;
    if ((u32)y > SCREEN_BOTTOM)
        return;

    stride = gTextRowStride;
    dest = gTextVramBase + ((x >> TILE_SHIFT) << 6)
         + ((y >> TILE_SHIFT) * stride << 6);

    control = ((height << 6) >> 1) | 0x81000000;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = dest;
    REG_DMA3.control = control;
    REG_DMA3.control;
    REG_IME = ime;

    if (gHalfLineSpacing == 0) {
        dest += stride << 6;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        REG_DMA3.src = &fill;
        REG_DMA3.dst = dest;
        REG_DMA3.control = control;
        REG_DMA3.control;
        REG_IME = ime;
    }
}
