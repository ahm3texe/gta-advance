/* Harita karosu sorgulari — 0x08042434-0x0804246F
 *
 * Dunya haritasi 0x0202F3E0'daki baglamdan okunuyor: +0 karo verisi,
 * +0x38 satir kaydirma miktari. Konumlar 22 bit kesirli; karo indeksi
 * (y << shift) + x.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/map_tiles.c
 */

#include "gba_types.h"

/* Karo turu: dusuk dort bit tur, 0x70 bitleri ayrilmis. */
#define TILE_TYPE_MASK     0x000F
#define TILE_BLOCKED_MASK  0x0070
#define TILE_TYPE_MAX      4

typedef struct MapPos {
    s32 x;                  /* +0  22 bit kesirli */
    s32 y;                  /* +4 */
} MapPos;

typedef struct MapData {
    u8   pad0[4];
    u16 *tiles;             /* +4 */
} MapData;

typedef struct MapContext {
    MapData *data;          /* +0 */
    u8       pad4[0x34];
    int      tileShift;     /* +0x38  satir basina karo kaydirmasi */
} MapContext;

extern MapContext gRam0202F3E0;

/* 0x08042434 — karo turu 1..4 araliginda ve engel bitleri bos mu. */
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
