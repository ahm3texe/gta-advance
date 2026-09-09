/* Bits 0x380 of the tile at a position — 0x08041E98-0x08041EC7
 *
 * The same tile lookup as src/world/map_tiles.c and src/world/map_tile_fields.c:
 * positions carry 22 fractional bits, and the tile index is (y << shift) + x.
 * This one takes bits 7 to 9 and shifts them down, so the field is three bits
 * wide and sits above the 0x70 field src/world/map_tiles.c reads.
 *
 * 0x380 is `movs #224 / lsls #2`, and the shift down is an ARITHMETIC one in
 * the ROM even though the masked value cannot be negative; that is what an
 * `int` result gives.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/get_tile_field_c.c
 */

#include "gba_types.h"

#define FIELD_C_MASK   (224 << 2)   /* 0x380 */
#define FIELD_C_SHIFT  7
#define POS_SHIFT      22

typedef struct MapPos {
    s32 x;                      /* +0x00, 22 fractional bits */
    s32 y;                      /* +0x04 */
} MapPos;

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

/* 0x08041E98 */
s32 FUN_08041e98(const MapPos *pos)
{
    u16 *tiles;
    int x;
    int y;
    s32 tile;

    x = pos->x >> POS_SHIFT;
    y = pos->y >> POS_SHIFT;
    tiles = gRam0202F3E0.data->tiles;
    tile = *(tiles + x + (y << gRam0202F3E0.tileShift));
    return (tile & FIELD_C_MASK) >> FIELD_C_SHIFT;
}
