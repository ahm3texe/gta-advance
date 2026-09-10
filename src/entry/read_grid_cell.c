/* Read a grid cell by two fixed-point coordinates — 0x0801FF40-0x0801FF63
 *
 * Both coordinates come down with an ARITHMETIC shift of 22, so both are
 * signed. The cell index is `x + width * y`, and the ROM computes the product
 * FIRST, into its own register, before adding the base and x.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/read_grid_cell.c
 */

#include "gba_types.h"

#define COORD_SHIFT  22

typedef struct GridHeader {
    u16 width;                  /* +0x00 */
    u16 pad02;
    u8 *cells;                  /* +0x04 */
} GridHeader;

extern GridHeader *gRam020230E8;

/* 0x0801FF40 */
u32 FUN_0801ff40(s32 x, s32 y)
{
    GridHeader *grid;
    u32 row;
    u8 *cell;

    x = x >> COORD_SHIFT;
    y = y >> COORD_SHIFT;
    grid = gRam020230E8;
    row = grid->width * y;
    cell = grid->cells + x;
    cell = cell + row;
    return *cell;
}
