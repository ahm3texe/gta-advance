/* Metin cizim baglamini kurma — 0x0806430C-0x0806434B
 *
 * Alti parametreyi alti genel degiskene yaziyor; dallanma yok.
 * Dordu yazmacta (r0-r3), ikisi yiginda (sp+16, sp+20 -- push {r4,r5,r6,lr}
 * 16 bayt oldugu icin).  Besinci parametre `lsls #24 / lsrs #24` ile
 * u8'e kirpilıyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * ADLANDIRMA NOTU — cozulmus celiski:
 * Cagiranlar (menu_screen.c, menu_loop.c) bu fonksiyonu KARO YUKLEYICI
 * saniyordu (`tilesA`, `tilesB` parametre adlari).  Disassembly ise saf
 * bir baglam kurucusu gosteriyor.  Karar KULLANIMDAN verildi:
 * `gGlyphWidths[index]` iki ESLESMIS dosyada dizi olarak indeksleniyor
 * (clear_text_area.c:115, draw_text.c:58), yani gercekten tablo
 * isaretcisi.  Cagirandaki `tilesB` adi yanlis tahmindi.
 *
 * ACIK SORU: `gFontIndex` bu cagrida 160 aliyor (MENU_TILE_WIDTH) ve
 * izleme logunda 160/192/128/240/242 degerlerini aliyor -- bunlar font
 * indeksinden cok genislik/konum gibi duruyor.  Ad SUPHELI ama daha iyi
 * bir ad icin kesin kanit yok; eslesmis dosyalara dokunmamak icin
 * korundu.
 *
 * YENI SEMBOL: 0x02036308 haritada yoktu.  Cagrida 0x08831880 aliyor;
 * gGlyphWidths'in aldigi 0x0883F880 ile arasi tam 0xE000 bayt, yani
 * once glif bitmapleri sonra genislik tablosu duzenine uyuyor.
 * `gGlyphTiles` adi bu nedenle GECICI.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/text/set_text_context.c
 */

#include "gba_types.h"

extern u8  *gTextVramBase;      /* 0x02036310 */
extern u32  gTextRowStride;     /* 0x0203630C */
extern u8  *gGlyphTiles;        /* 0x02036308 -- GECICI ad */
extern u8  *gGlyphWidths;       /* 0x02036314 */
extern u32  gFontIndex;         /* 0x02036318 -- ad supheli, yukari bak */
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
