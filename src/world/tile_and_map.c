/* Karo alan getirici (2. deger) ve harita boyutlari — 0x080424A8-0x080424E5
 *
 * Ilk fonksiyon src/world/map_tile_fields.c'deki GetTileFieldA'nin
 * kardesi (ayni yapi, farkli konum). Ikincisi harita baglaminin ilk
 * iki u16'sini (genislik/yukseklik olabilir) iki dest'e yaziyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/tile_and_map.c
 */

#include "gba_types.h"

#define FIELD_A_MASK    0x0380
#define FIELD_A_SHIFT   7

typedef struct MapPos {
    s32 x;
    s32 y;
} MapPos;

typedef struct MapData {
    u16 width;                  /* +0x00 */
    u16 height;                 /* +0x02 */
    u16 *tiles;                 /* +0x04 */
} MapData;

typedef struct MapContext {
    MapData *data;              /* +0x00 */
    u8       pad4[0x34];
    int      tileShift;         /* +0x38 */
} MapContext;

extern MapContext gRam0202F3E0;

/* 0x080424A8 */
u32 GetTileFieldA2(const MapPos *pos)
{
    u16 *tiles;
    int x;
    int y;
    u32 tile;

    tiles = gRam0202F3E0.data->tiles;
    x = pos->x >> 22;
    y = pos->y >> 22;
    tile = *(tiles + x + (y << gRam0202F3E0.tileShift));

    return (tile & FIELD_A_MASK) >> FIELD_A_SHIFT;
}

/* 0x080424D8 */
void GetMapSize(u32 *widthOut, u32 *heightOut)
{
    MapData *data;

    data = gRam0202F3E0.data;
    *widthOut = data->width;
    *heightOut = data->height;
}
