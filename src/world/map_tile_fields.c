/* Map tile field getters — 0x08042058-0x080420AF
 *
 * Same structure as IsTileTypeInRange in map_tiles.c: gRam0202F3E0 context
 * (+0 tile data, +0x38 row shift), positions with 22 fractional bits, and
 * pointer arithmetic *(tiles + x + (y << shift)).
 *
 * The functions extract different fields:
 *   0x08042058: (tile & 0x380) >> 7 — middle three bits
 *   0x08042088: tile & 0x0F — low four bits (type)
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/map_tile_fields.c
 */

#include "gba_types.h"

#define FIELD_A_MASK    0x0380
#define FIELD_A_SHIFT   7
#define TYPE_MASK       0x000F

typedef struct MapPos {
    s32 x;                      /* +0, 22 fractional bits */
    s32 y;                      /* +4 */
} MapPos;

typedef struct MapData {
    u8   pad0[4];
    u16 *tiles;                 /* +4 */
} MapData;

typedef struct MapContext {
    MapData *data;              /* +0x00 */
    u8       pad4[0x34];
    int      tileShift;         /* +0x38, row-to-tile shift */
    u8       pad3C[0x98];
    u32      flags;             /* +0xD4 */
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
