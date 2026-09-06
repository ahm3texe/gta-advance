/* Hedefe dogru kaydirilmis konumu dene, tutmazsa komsu karolari tara
 * 0x08055054-0x0805518B  (312 bayt; son 4 bayt literal havuzu)
 *
 * Cagriya bir konum isaretcisi (in/out) ve bir (dx, dy) kaymasi geliyor.
 * Once `konum + (dx,dy)<<16` noktasi FUN_08054f1c ile snaniyor; bos ise
 * konum oraya tasinip 1 donuluyor. Degilse istenen nokta cevresinde
 * +-128.0 (0x800000) genisliginde bir kutu kurulup FUN_08040700'e
 * veriliyor; o da kutuya giren en fazla 8 karonun (tx, ty) indekslerini
 * yaziyor. Her karo icin karo merkezi (tx<<22 + dx<<16 + 32.0) tekrar
 * snaniyor; ilk bos karo bulundugunda konumun x/y'si oraya cekilip 1
 * donuluyor. Hicbiri tutmazsa 0.
 *
 * Tarama en fazla IKI tur: ilk turda (yalniz useTileMask ise) oyuncunun
 * uzerinde durdugu karo turunun maskesi (1 << GetTileFieldA2), ikinci
 * turda cagricinin verdigi maske kullaniliyor. Iki maske ayni ciktiysa
 * ya da useTileMask sifirsa tek tur yetiyor.
 *
 * YAPI IPUCLARI:
 *   - Konum 12 baytlik {s32 x, y, z}: ilk basarida `ldmia/stmia {r2,r3,r4}`
 *     ile tek seferde kopyalaniyor (kural 32 -- struct atamasi).
 *   - Kutu iki ayri Vec3: FUN_08040700 onu [r0,#0]/[r0,#4] ve
 *     [r0,#12]/[r0,#16] diye okuyor, yani 12 bayt adimli 2'lik dizi.
 *   - Karo tamponu 8 x {u16 tx, u16 ty} = 32 bayt (sp+40..71).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b7.c
 *
 * DENENENLER (elenen yollari SILME, yenilerini EKLE):
 *   - (1. tur) duz yapisal yazim; olcum asagida.
 */

#include "gba_types.h"

#define BOX_RADIUS      0x00800000      /* 128.0, 16.16 sabit nokta */
#define HALF_TILE       0x00200000      /*  32.0 = karo yarisi      */
#define PROBE_LIMIT     0x00180000      /*  24.0; FUN_08054f1c esigi */
#define MAX_CELLS       8
#define TILE_SHIFT      22              /* karo indeksi -> dunya kord. */
#define POS_SHIFT       16              /* tam sayi -> 16.16          */

/* Dunya konumu; z alani tasiniyor ama snamada kullanilmiyor. */
typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

/* FUN_08040700'un yazdigi karo indeksi cifti. */
typedef struct Cell {
    u16 tx;                     /* +0x00 */
    u16 ty;                     /* +0x02 */
} Cell;

extern u32 GetTileFieldA2(const Vec3 *pos);
extern u32 FUN_08054f1c(const Vec3 *pos, u32 mask, u32 limit);
extern int FUN_08040700(const Vec3 *box, Cell *out, int max, u32 mask, u32 opt);

/* 0x08055054 */
u32 FUN_08055054(Vec3 *pos, u32 mask, u32 useTileMask, u32 probeMask,
                 s32 dx, s32 dy)
{
    Vec3 want;
    Vec3 box[2];
    Cell cells[MAX_CELLS];
    Vec3 probe;
    u32  sel;
    u32  tileMask;
    u32  fallback;
    s32  yoff;
    s32  xoff;
    int  attempt;
    int  count;
    int  i;

    want.x = pos->x + (dx << POS_SHIFT);
    want.y = pos->y + (dy << POS_SHIFT);
    want.z = pos->z;
    tileMask = 1 << GetTileFieldA2(pos);

    if (useTileMask != 0) {
        if (FUN_08054f1c(&want, probeMask, PROBE_LIMIT)) {
            *pos = want;
            return 1;
        }
    }

    box[0].x = want.x - BOX_RADIUS;
    box[1].x = want.x + BOX_RADIUS;
    box[0].y = want.y - BOX_RADIUS;
    box[1].y = want.y + BOX_RADIUS;

    fallback = mask;
    for (attempt = 0; attempt <= 1; attempt++) {
        sel = fallback;
        if (useTileMask != 0 && attempt == 0)
            sel = tileMask;

        count = FUN_08040700(box, cells, MAX_CELLS, sel, 0);
        for (i = 0; i < count; i++) {
            xoff = (dx << POS_SHIFT) + HALF_TILE;
            yoff = (dy << POS_SHIFT) + HALF_TILE;
            probe.x = (cells[i].tx << TILE_SHIFT) + xoff;
            probe.y = (((u16 *)cells)[i * 2 + 1] << TILE_SHIFT) + yoff;
            probe.z = 0;
            if (FUN_08054f1c(&probe, probeMask, PROBE_LIMIT)) {
                pos->x = probe.x;
                pos->y = probe.y;
                return 1;
            }
        }

        if (useTileMask == 0)
            break;
        if (tileMask == fallback)
            break;
    }

    return 0;
}
