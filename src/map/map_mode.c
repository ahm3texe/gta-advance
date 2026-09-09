/* Which of three map modes — 0x080420F8-0x0804211B
 *
 * The +0xD4 word answers 3 when bit 0 is set, 2 when it is non-zero without
 * bit 0, and 1 when it is zero. The +0xD4 read is what fixes the map context's
 * extent in data/ram_map.csv, which had only reached +0x38 before.
 *
 * Rule 33: the ROM materialises the 1 first and ands the word into it
 * (`movs r0,#1 / ands r0,r1`), which leaves the word itself intact in r1 for
 * the second test.
 *
 * The base goes through the ROM's own `adds r0,#212` rather than a folded pool
 * constant, which is what a struct field of a symbol already gives.
 *
 * The last two answers are a `goto`, with the 2 written AFTER the label so that
 * it is the one that lands first in the ROM; src/script/cmd_area_ready.c and
 * src/entity/is_lookup_nonnegative.c record the same direction. A two-armed if
 * with a result variable does not work here: agbcc hoists the 1 above the test
 * and overwrites it, which is rule 48's shape and one instruction shorter than
 * the ROM's.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/map_mode.c
 */

#include "gba_types.h"

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

/* 0x080420F8 */
u32 FUN_080420f8(void)
{
    u32 flags = gRam0202F3E0.flags;
    u32 probe = 1;

    probe &= flags;
    if (probe != 0)
        return 3;
    if (flags != 0) goto two;
    return 1;
two:
    return 2;
}
