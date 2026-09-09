/* Finding a random tile of a given kind — 0x080424EC-0x08042573
 *
 * From the tile map of the gRam0202F3E0 context (+0 header: u16 width, u16
 * height, +4 the tile array; the row shift at the context's +0x38), it picks a
 * random (x,y) at least 8 tiles inside the edges, at most 256 attempts; if the
 * tile's kind (masked with 0x380, >>7) equals the requested one it writes the
 * position and returns 1, otherwise 0.
 *
 * TWO MEASUREMENTS: gRam0202F3E0 is NOT a pointer but a context whose +0 is a
 * pointer (the shift is re-read from +0x38 of the same block on every round);
 * MapData's +0/+2 u16 width/height fields are read through a cast because they
 * are padding in the shared body.
 * The tile address must be written as TWO SEPARATELY SCALED TERMS,
 * `(&tiles[x])[y << shift]`; `tiles[x + (y << shift)]` folds the sum first and
 * diverges by 17 instructions.  A mask local is not required (both match).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/find_random_tile_of_kind.c
 */

#include "gba_types.h"
#define TRIES     256
#define KIND_MASK 0x380
/* The same view as in map_tiles.c (the consistency gate): the width and
 * height are in MapData's first four bytes, read here as a u16 array. */
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
extern u32 FUN_08032548(void);
u32 FindRandomTileOfKind(u32 kind, u32 *outX, u32 *outY)
{
    MapData *hdr; u16 *tiles; u32 w; u32 h; s32 i; u32 x; u32 y; s32 t;
    hdr = gRam0202F3E0.data;
    tiles = hdr->tiles;
    w = ((u16 *)hdr)[0];
    h = ((u16 *)hdr)[1];
    for (i = 0; i <= TRIES - 1; i++) {
        x = ((FUN_08032548() * (w - 16)) >> 16) + 8;
        y = ((FUN_08032548() * (h - 16)) >> 16) + 8;
        t = (&tiles[x])[y << gRam0202F3E0.tileShift];
        if (kind == ((KIND_MASK & t) >> 7)) {
            *outX = x;
            *outY = y;
            return 1;
        }
    }
    return 0;
}
