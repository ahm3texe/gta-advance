/* On alti BGR555 rengi 16.16 karisim katsayisiyla ara degerler.
 *
 * Her kanal ayri olarak 5 bite ayrilir, iki palet arasindaki fark katsayi
 * ile carpilir ve sonuc yeniden BGR555 kelimesine paketlenir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm
 * Dogrulama: make c-match FILE=src/world/palette_lerp.c
 */

#include "gba_types.h"

/* 0x080199BC */
void BlendPaletteBlock(const u16 *from, const u16 *to, u16 *dst, s32 blend)
{
    int i;
    u32 mask;

    mask = 31;
    i = 15;
    do {
        u16 a;
        u16 b;
        s32 ar;
        s32 ag;
        s32 ab;
        s32 br;
        s32 bg;
        s32 bb;

        a = *from;
        ar = a & 31;
        ag = (a >> 5) & mask;
        ab = (a >> 10) & mask;
        b = *to;
        br = b & 31;
        bg = (b >> 5) & mask;
        bb = (b >> 10) & mask;

        br -= ar;
        bg -= ag;
        bb -= ab;
        ar += blend * br >> 16;
        ag += bg * blend >> 16;
        ab += bb * blend >> 16;
        *dst = (ab << 10) | (ag << 5) | ar;

        i--;
        from++;
        to++;
        dst++;
    } while (i >= 0);
}
