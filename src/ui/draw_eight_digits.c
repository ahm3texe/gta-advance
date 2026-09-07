/* Sekiz basamakli sayaci iki satirlik karolarla yazma
 * — 0x0802A610-0x0802A857
 *
 * IKI FONKSIYON, AYNI GOVDE.  tools/find_twins.py %99.3 verdi; ROM
 * govdeleri komut komut ayni, yalnizca havuz adresleri kayik -- ayni
 * kaynak iki kez derlenmis (docs/WORKFLOW.md §10).
 *
 * DrawTwoDigits (src/ui/draw_two_digits.c, 0x08031498) ile ayni karo
 * sozlugu: ust satir 0xF8+d, alt satir 0x102+d, ikisi de 0xF000 palet
 * nibble'iyla OR'lu; bos karo 0xF0E8.  Fark: burada sekiz basamak var,
 * taban 0x0600980E ve bastaki sifirlar 10 ile isaretlenip bos karoya
 * cevriliyor.
 *
 * UC OLCUM:
 *   - Yazma dongusu ARTAN indisle yazilmali: `for (i = 0; i < 8; i++)`
 *     ve `digits[i]`.  Azalan sayac + `digits[7-i]` ayni komutlari
 *     veriyor ama `movs r5,#7` bir komut erken cikiyor (1 komut fark);
 *     isaretci yurutmek 20 komut fark veriyor.
 *   - Bos karo sabiti bastaki sifir dongusunde AYRI YERELE alinmali;
 *     dogrudan yazilirsa adres hesabi sabitten once uretiliyor.
 *   - Bastaki sifir dongusunun sinir karsilastirmasi ISARETLI olmali
 *     (ROM `ble`); isaretci karsilastirmasi `bls` uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/draw_eight_digits.c
 */

#include "gba_types.h"

extern s32 Div(s32 numerator, s32 denominator);

#define TILE_MAP_BASE   0x0600980E
#define TILE_ROW_STEP   64
#define DIGIT_TOP       0xF8
#define DIGIT_BOTTOM    0x102
#define DIGIT_BLANK     10
#define TILE_ATTR       0xF000
#define TILE_BLANK      0xF0E8

/* 0x0802A610 */
void DrawCounterDigits8(s32 value, s32 x, s32 y)
{
    s32 digits[8];
    s32 *p;
    vu16 *base;
    vu16 *top;
    vu16 *bot;
    s32 col;
    s32 rowoff;
    s32 d;
    s32 i;
    s32 blank;

    digits[7] = Div(value, 10000000);
    value -= digits[7] * 10000000;
    digits[6] = Div(value, 1000000);
    value -= digits[6] * 1000000;
    digits[5] = Div(value, 100000);
    value -= digits[5] * 100000;
    digits[4] = Div(value, 10000);
    value -= digits[4] * 10000;
    digits[3] = Div(value, 1000);
    value -= digits[3] * 1000;
    digits[2] = Div(value, 100);
    value -= digits[2] * 100;
    digits[1] = Div(value, 10);
    value -= digits[1] * 10;
    digits[0] = value;

    if (digits[7] == 0) {
        blank = DIGIT_BLANK;
        p = &digits[7];
        do {
            *p = blank;
            p--;
        } while ((s32)p > (s32)&digits[0] && *p == 0);
    }

    col = x * 2;
    rowoff = y * TILE_ROW_STEP;
    base = (vu16 *)TILE_MAP_BASE;
    top = (vu16 *)(col + ((u32)base + rowoff));
    rowoff = (y + 1) * TILE_ROW_STEP;
    bot = (vu16 *)(col + ((u32)base + rowoff));

    for (i = 0; i < 8; i++) {
        d = digits[i];
        if (d == DIGIT_BLANK) {
            *top-- = TILE_BLANK;
            *bot-- = TILE_BLANK;
        } else {
            *top-- = (DIGIT_TOP + d) | TILE_ATTR;
            *bot-- = (DIGIT_BOTTOM + d) | TILE_ATTR;
        }
    }
}

/* 0x0802A734 — AYNI GOVDE; TEK FARK dongu sayisi 8 yerine 3.
 * (ROM'da `movs r5,#7` yerine `movs r5,#2`.)  Sekiz basamak yine
 * hesaplaniyor, yalnizca uc tanesi ciziliyor. */
void DrawCounterDigits3(s32 value, s32 x, s32 y)
{
    s32 digits[8];
    s32 *p;
    vu16 *base;
    vu16 *top;
    vu16 *bot;
    s32 col;
    s32 rowoff;
    s32 d;
    s32 i;
    s32 blank;

    digits[7] = Div(value, 10000000);
    value -= digits[7] * 10000000;
    digits[6] = Div(value, 1000000);
    value -= digits[6] * 1000000;
    digits[5] = Div(value, 100000);
    value -= digits[5] * 100000;
    digits[4] = Div(value, 10000);
    value -= digits[4] * 10000;
    digits[3] = Div(value, 1000);
    value -= digits[3] * 1000;
    digits[2] = Div(value, 100);
    value -= digits[2] * 100;
    digits[1] = Div(value, 10);
    value -= digits[1] * 10;
    digits[0] = value;

    if (digits[7] == 0) {
        blank = DIGIT_BLANK;
        p = &digits[7];
        do {
            *p = blank;
            p--;
        } while ((s32)p > (s32)&digits[0] && *p == 0);
    }

    col = x * 2;
    rowoff = y * TILE_ROW_STEP;
    base = (vu16 *)TILE_MAP_BASE;
    top = (vu16 *)(col + ((u32)base + rowoff));
    rowoff = (y + 1) * TILE_ROW_STEP;
    bot = (vu16 *)(col + ((u32)base + rowoff));

    for (i = 0; i < 3; i++) {
        d = digits[i];
        if (d == DIGIT_BLANK) {
            *top-- = TILE_BLANK;
            *bot-- = TILE_BLANK;
        } else {
            *top-- = (DIGIT_TOP + d) | TILE_ATTR;
            *bot-- = (DIGIT_BOTTOM + d) | TILE_ATTR;
        }
    }
}
