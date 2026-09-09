#ifndef GUARD_MAP_GRID_H
#define GUARD_MAP_GRID_H

#include "gba_types.h"

/* Map grid pointed to by gRam0201AEE8.
 *
 * The layout was established from 0x080651E0: bounds checks read +0x00 and
 * +0x02 as u16 values, and the pointer at +0x04 is indexed by `y * width + x`
 * to read a u16 tile value.
 *
 * Users of this symbol include:
 *   src/world/slot_probe.c    reads grid fields and tile data
 *   src/world/window_config.c only assigns the pointer
 * This shared definition avoids conflicting extern types for the same symbol,
 * which are rejected by the consistency check.
 */
typedef struct Grid {
    u16  width;                 /* +0x00 */
    u16  height;                /* +0x02 */
    u16 *tiles;                 /* +0x04 */
} Grid;

extern Grid *gRam0201AEE8;

#endif /* GUARD_MAP_GRID_H */
