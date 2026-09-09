/* Bits 0x70 of a tile, by whole coordinates — 0x0804211C-0x08042143
 *
 * The sibling of src/map/get_tile_field_c.c over the same tile word, but the
 * coordinates arrive as two u16 already in tile units, so there is no shift
 * down by 22. It reads the same 0x70 field src/world/map_tiles.c tests for
 * zero, and returns it as a value rather than a flag.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/get_tile_field_b.c
 */

#include "gba_types.h"

#define FIELD_B_MASK   0x70
#define FIELD_B_SHIFT  4

typedef struct TilePos {
    u16 x;                      /* +0x00 */
    u16 y;                      /* +0x02 */
} TilePos;

typedef struct MapData {
    u16 width;                  /* +0x00 */
    u16 height;                 /* +0x02 */
    u16 *tiles;                 /* +0x04 */
} MapData;

typedef struct MapContext {
    MapData *data;              /* +0x00 */
    u8       pad4[0x34];
    int      tileShift;         /* +0x38, row-to-tile shift */
    u8       pad3C[0x98];
    u32      flags;             /* +0xD4 */
} MapContext;

extern MapContext gRam0202F3E0;

/* 0x0804211C */
u32 FUN_0804211c(const TilePos *pos)
{
    u16 *tiles;
    u32 tile;

    tiles = gRam0202F3E0.data->tiles;
    tile = *(tiles + pos->x + (pos->y << gRam0202F3E0.tileShift));
    return (tile & FIELD_B_MASK) >> FIELD_B_SHIFT;
}
