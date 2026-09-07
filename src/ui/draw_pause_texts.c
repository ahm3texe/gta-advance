/* Duraklatma metinleri — 0x080023F0-0x080024B3 (196 bayt)
 *
 * DURUM: 46/85 komut, YAKIN ISKA (eslesmiyor). Boyut tutuyor.
 *
 * ClearTextArea(0,64,10); secime gore SetTextContext (0x06008240, 30,
 * karo A/B, genislik B, 128, bayrak) ve DrawText(GetTextString(148), 0,
 * 64); ayni sey `sel == 0` bayragiyla ve metin 149 (40,64) icin.
 *
 * KALAN FARK: ROM SetTextContext argumanlarini r2,r3, yigin, EN SON r0
 * (VRAM havuz sabiti) sirasiyla yukluyor; agbcc bende VRAM'i ILK
 * yukluyor ve `ok`/`sel` icin iki callee-saved yazmac ayiriyor (ROM
 * yalnizca sel r4, ok r1). Denenen: VRAM void* / u8* / tamsayi makro, karo
 * tablolari cast makrosu vs extern dizi, `ok` yereli / satir ici
 * `sel == 0` / makro ile iki kez acilim (42-46). menu_loop.c'deki
 * eslesen cagrida VRAM 0x06000000 (movs/lsls ile uretiliyor, havuz
 * yok); buradaki fark havuz sabitinin arguman sirasinda nereye
 * kondugu -- bilinen kaynak kaldiraci yok.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/draw_pause_texts.c
 */

#include "gba_types.h"
#define VRAM            ((void *)0x06008240)
#define MENU_TILE_COUNT 30
#define MENU_TILE_WIDTH 128
#define TILES_A         ((const void *)0x08831880)
#define WIDTHS_A        ((const void *)0x0883F880)
#define TILES_B         ((const void *)0x0883FB60)
#define WIDTHS_B        ((const void *)0x0884DB60)
#define TEXT_RESUME     148
#define TEXT_QUIT       149
extern void ClearTextArea(u32 x, u32 y, u32 rows);
extern void SetTextContext(u8 *vram, u32 stride, u8 *tiles, u8 *widths, u32 width, u32 sel);
extern u32  GetTextString(u32 index);
extern void DrawText(u32 text, int x, int y);
void DrawPauseTexts(u32 sel)
{
    ClearTextArea(0, 64, 10);
    if (sel != 0)
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_A, WIDTHS_A, MENU_TILE_WIDTH, 0);
    else
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_B, WIDTHS_B, MENU_TILE_WIDTH, sel);
    DrawText(GetTextString(TEXT_RESUME), 0, 64);
    if (sel == 0)
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_A, WIDTHS_A, MENU_TILE_WIDTH, 0);
    else
        SetTextContext(VRAM, MENU_TILE_COUNT, TILES_B, WIDTHS_B, MENU_TILE_WIDTH, sel == 0);
    DrawText(GetTextString(TEXT_QUIT), 40, 64);
}
