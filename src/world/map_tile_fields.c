/* Harita karosu alan getiricileri — 0x08042058-0x080420AF
 *
 * `map_tiles.c`'deki IsTileTypeInRange ile ayni yapi: gRam0202F3E0
 * baglami (+0 karo verisi, +0x38 satir kaydirmasi), 22-bit kesirli
 * (x, y) konum, isaretci aritmetigi `*(tiles + x + (y << shift))`.
 *
 * Bu iki fonksiyon karonun farkli bit alanlarini cikariyor:
 *   0x08042058: (karo & 0x380) >> 7  — orta 3 bit
 *   0x08042088: karo & 0x0F         — dusuk 4 bit (tur)
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/map_tile_fields.c
 */

#include "gba_types.h"

#define FIELD_A_MASK    0x0380
#define FIELD_A_SHIFT   7
#define TYPE_MASK       0x000F

typedef struct MapPos {
    s32 x;                      /* +0  22 bit kesirli */
    s32 y;                      /* +4 */
} MapPos;

typedef struct MapData {
    u8   pad0[4];
    u16 *tiles;                 /* +4 */
} MapData;

typedef struct MapContext {
    MapData *data;              /* +0 */
    u8       pad4[0x34];
    int      tileShift;         /* +0x38 */
} MapContext;

extern MapContext gRam0202F3E0;

/* 0x08042058 */
u32 GetTileFieldA(const MapPos *pos)
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

/* 0x08042088 */
u32 GetTileType(const MapPos *pos)
{
    u16 *tiles;
    int x;
    int y;
    u32 tile;

    tiles = gRam0202F3E0.data->tiles;
    x = pos->x >> 22;
    y = pos->y >> 22;
    tile = *(tiles + x + (y << gRam0202F3E0.tileShift));

    return tile & TYPE_MASK;
}
