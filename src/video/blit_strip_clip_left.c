/* 8x48 4bpp serit cizici, SOL kirpma — 0x08031844-0x08031A1B
 *
 * src/video/blit_strip_4bpp.c'deki 0x08031A1C ile AYNI KAYNAK; tek fark
 * sutun testinin yonu.
 *
 * NASIL BULUNDU (yeni denemeden once bunu yapin): iki ROM govdesinin
 * komut listeleri karsilastirildi --
 *     python3 tools/disasm_function.py 0x08031844 > a
 *     python3 tools/disasm_function.py 0x08031A1C > b
 *     diff a b        # yalnizca DAL komutlari farkli
 * 235 komutun 235'i ayni; fark yalnizca dal hedefleri ve sutun testinin
 * kosulu (`blt` -> `bge`). Yani 0x08031A1C `col >= 0` iken duz kaynagi
 * aliyor, bu fonksiyon `col < 0` iken aliyor -- seridin ters kenari.
 * Kaynakta karsiligi tek karakter: `if (col++ >= 0)` -> `if (col++ < 0)`.
 *
 * Bu ders genellenebilir: bir fonksiyon eslesmiyorsa ve ROM'da AYNI
 * BOYUTA yakin bir kardesi zaten eslesiyorsa, once iki govdeyi
 * birbiriyle diff'leyin. Yazmac dagitimi kovalamadan once bu bakilmali.
 *
 * Kardesten devralinan olcumler (hepsi burada da gecerli):
 *  1. Dongu AZALAN; satir `rowBase + (48 - i)` diye YENIDEN hesaplaniyor.
 *  2. Dikey kirpma testi TEK `if (a || b)`.
 *  3. `(*mask & *under)` sirasi onemli: agbcc AND'in IKINCI operandini
 *     once yukluyor.
 *  4. HIZLI YOLDA mask/under NIBBLE BASINA artirilmali; ROM'daki tek
 *     `+= 8`'i derleyici dagitimdan SONRA kendisi uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/video/blit_strip_clip_left.c
 */

#include "gba_types.h"

#define STRIP_ROWS   48
#define ROW_BYTES    40

/* 0x08031844 */
void BlitStripClipLeft4bpp(s32 rowBase, s32 colBase, s32 rowLimit,
                           s32 maskStep, const u8 *mask, const u8 *under,
                           u16 *dst, const u8 *src)
{
    s32 i;
    s32 row;
    s32 col;
    u32 n0;
    u32 n1;
    u32 n2;
    u32 n3;

    for (i = STRIP_ROWS; i != 0; i--) {
        col = colBase;
        row = rowBase + (STRIP_ROWS - i);
        if (row >= rowLimit || row < 0) {
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        } else {
            if (col++ < 0) {
                n0 = *src;
                src++;
                mask++;
                under++;
            } else {
                n0 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n1 = *src;
                src++;
                mask++;
                under++;
            } else {
                n1 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n2 = *src;
                src++;
                mask++;
                under++;
            } else {
                n2 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n3 = *src;
                src++;
                mask++;
                under++;
            } else {
                n3 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            if (col++ < 0) {
                n0 = *src;
                src++;
                mask++;
                under++;
            } else {
                n0 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n1 = *src;
                src++;
                mask++;
                under++;
            } else {
                n1 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n2 = *src;
                src++;
                mask++;
                under++;
            } else {
                n2 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            if (col++ < 0) {
                n3 = *src;
                src++;
                mask++;
                under++;
            } else {
                n3 = (*mask & *under) | *src;
                under++;
                mask++;
                src++;
            }
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        }
        mask += maskStep;
        under += ROW_BYTES;
        src += ROW_BYTES;
    }
}
