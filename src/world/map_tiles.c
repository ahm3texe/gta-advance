/* Map tile queries — 0x08042434-0x0804246F
 *
 * Read the world map through the context at 0x0202F3E0: +0 tile data, +0x38
 * row shift. Positions have 22 fractional bits; tile index = (y << shift) + x.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/map_tiles.c
 */

#include "gba_types.h"

/* Tile type: low four bits hold the type; bits 0x70 are reserved. */
#define TILE_TYPE_MASK     0x000F
#define TILE_BLOCKED_MASK  0x0070
#define TILE_TYPE_MAX      4

typedef struct MapPos {
    s32 x;                  /* +0, 22 fractional bits */
    s32 y;                  /* +4 */
} MapPos;

typedef struct MapData {
    u8   pad0[4];
    u16 *tiles;             /* +4 */
} MapData;

typedef struct MapContext {
    MapData *data;              /* +0x00 */
    u8       pad4[0x34];
    int      tileShift;         /* +0x38, row-to-tile shift */
    u8       pad3C[0x98];
    u32      flags;             /* +0xD4 */
} MapContext;

extern MapContext gRam0202F3E0;

/* 0x08042434 — test type 1..4 with obstacle bits clear. */
u32 IsTileTypeInRange(const MapPos *pos)
{
    u16 *tiles;
    int x;
    int y;
    u32 tile;

    tiles = gRam0202F3E0.data->tiles;
    x = pos->x >> 22;
    y = pos->y >> 22;
    tile = *(tiles + x + (y << gRam0202F3E0.tileShift));

    if ((tile & TILE_BLOCKED_MASK) != 0)
        return 0;

    tile &= TILE_TYPE_MASK;
    if (tile == 0)
        return 0;
    if (tile > TILE_TYPE_MAX)
        return 0;

    return 1;
}
