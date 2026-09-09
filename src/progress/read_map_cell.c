/* Top six bits of a map cell — 0x080313F0-0x08031413
 *
 * The +0x30 word points at a header holding the row width at +0x00 and the cell
 * array at +0x04. Both coordinates arrive in a fixed-point form and are brought
 * down with an ARITHMETIC shift of 22, so both are signed.
 *
 * Rule 65 for the base and rule 70 for the cell array. Applied to the symbol
 * directly, `gRam02025810 + 0x30` folds into the pool constant and the load
 * loses its displacement; and the cell array taken at its point of use is
 * loaded after the first coordinate is shifted, where the ROM loads it before.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/read_map_cell.c
 */

#include "gba_types.h"

#define MAP_OFFSET   0x30
#define COORD_SHIFT  22
#define CELL_SHIFT   10

typedef struct MapHeader {
    u16  width;                 /* +0x00 */
    u16  pad02;
    u16 *cells;                 /* +0x04 */
} MapHeader;

extern u8 gRam02025810[];

/* 0x080313F0 */
u32 FUN_080313f0(s32 x, s32 y)
{
    u8 *base = gRam02025810;
    MapHeader *map = *(MapHeader **)(base + MAP_OFFSET);
    u16 *cell = map->cells;

    cell += x >> COORD_SHIFT;
    cell += map->width * (y >> COORD_SHIFT);
    return *cell >> CELL_SHIFT;
}
