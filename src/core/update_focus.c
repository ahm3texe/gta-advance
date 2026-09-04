/* Odak noktasi guncelleme — 0x0800A9E4-0x0800AA3B
 *
 * gGameState +0x0C bayragi kuruluysa VE ikinci hedef varsa iki hedefin
 * konumlarinin ORTA NOKTASINI, aksi halde birinci hedefin konumunu
 * gFocusPoint'e yaziyor. Bolme `asrs #1`, yani isaretli.
 *
 * ROM blok tabanini `adds r0,r3,#0` ile AYRI bir register'a kopyaliyor
 * ve orta nokta yolunda o kopyayi kullaniyor (kural 37).
 *
 * CoordBlock tanimi src/misc/coord_accessors.c ile BIREBIR AYNI olmali;
 * +0x04 ve +0x08 hedef isaretcisi olarak cast ediliyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * HENUZ ESLESMIYOR: bizim cikti 84 bayt, ROM 88. Sira DUZELDI (once
 * gGameState bayragi okunuyor, blok tabani sonra yukleniyor -- ROM'la
 * ayni sekil), kalan tek eksik TABAN KOPYASI:
 *     ROM  : adds r0,r3,#0  /  ldr r1,[r0,#8]
 *     bizim:                   ldr r1,[r2,#8]
 * `probe = block;` yazmak yetmiyor, agbcc duz isaretci kopyasini
 * BIRLESTIRIYOR.
 *
 * BU UCUNCU VAKA: ayni davranis src/text/clear_text_area.c (`dma2 = dma`)
 * ve src/world/set_bg1_enable.c (`mask = bit`) icin de olculdu. Buna
 * karsilik src/world/area_cleanup.c'deki `fill2 = fill` KOPYA URETTI --
 * aradaki fark, orada kopyanin iki AYRI STORE'da kullanilmasi; burada ve
 * oteki ikisinde kopya tek bir okuma icin isteniyor ve optimizer onu
 * gereksiz goruyor.
 *
 * Yani kural: kopyanin YASAMASI icin iki farkli KULLANIM YERI gerekiyor,
 * yalnizca ayri bir isim yetmiyor. Sonraki fikir bu yonde aranmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/update_focus.c
 */

#include "gba_types.h"

typedef struct Vec2 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
} Vec2;

typedef struct Target {
    u8    pad00[24];
    Vec2 *pos;                  /* +0x18 */
} Target;

typedef struct CoordBlock {
    u32 unk00;                  /* +0  */
    u32 unk04;                  /* +4  */
    u32 unk08;                  /* +8  */
    s32 second;                 /* +12 */
    s32 first;                  /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;                  /* +72 */
    u8  pad4C[37];
    u8  byte71;                 /* +0x71 */
} CoordBlock;

typedef struct GameState {
    u8 pad00[12];
    u8 flag;                    /* +0x0C */
} GameState;

extern CoordBlock gRam02011030;
extern GameState  gGameState;
extern Vec2       gFocusPoint;

/* 0x0800A9E4 */
void UpdateFocusPoint(void)
{
    CoordBlock *block;
    CoordBlock *probe;
    Target *first;
    Target *second;
    Vec2 *out;
    u32 flag;

    /* SIRA: ROM once gGameState bayragini OKUYOR, blok tabanini SONRA
       yukluyor. Ayrica bayrak yolunda tabani `adds r0,r3,#0` ile AYRI bir
       register'a KOPYALIYOR (kural 37) ve +8'i o kopyadan okuyor. */
    flag = gGameState.flag;
    block = &gRam02011030;
    if (flag != 0) {
        probe = block;
        second = (Target *)probe->unk08;
        if (second != 0)
            goto midpoint;
    }

    out = &gFocusPoint;
    first = (Target *)block->unk04;
    out->x = first->pos->x;
    out->y = first->pos->y;
    return;

midpoint:
    out = &gFocusPoint;
    first = (Target *)block->unk04;
    out->x = (first->pos->x + second->pos->x) >> 1;
    out->y = (first->pos->y + second->pos->y) >> 1;
}
