/* Centre two tile coordinates and call — 0x080423D0-0x080423F7
 *
 * Each of the two u16 coordinates is shifted up by 22 and half a tile,
 * 0x00200000, is added, so the result names the CENTRE of the tile rather than
 * its corner. The pair and a zero go into a three-word structure on the stack,
 * whose address is the callee's only argument.
 *
 * 0x00200000 is `movs #128 / lsls #14`, and it is built ONCE and reused for
 * both coordinates, which is what a local holding it gives. That local is
 * assigned AFTER the first coordinate is loaded and shifted, because that is
 * the order the ROM has them in; taken at the top it comes out first.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value, so the callee's
 * answer is this function's. With `pop {r0}; bx r0` it would be void.
 *
 * The `sub sp,#12` and `mov r0,sp` are the structure; there is no other reason
 * for this function to touch the stack.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/map/call_with_centred_pair.c
 */

#include "gba_types.h"

#define POS_SHIFT   22
#define HALF_TILE   (128 << 14)  /* 0x00200000 */

typedef struct TilePair {
    u16 pad00;
    u16 x;                      /* +0x02 */
    u16 pad04;
    u16 y;                      /* +0x06 */
} TilePair;

typedef struct MapPoint {
    s32 x;
    s32 y;
    s32 z;
} MapPoint;

extern u32 FUN_080519bc(const MapPoint *point);

/* 0x080423D0 */
u32 FUN_080423d0(u32 a, const TilePair *pair)
{
    MapPoint point;
    s32 x;
    s32 half;

    x = pair->x << POS_SHIFT;
    half = HALF_TILE;
    point.x = x + half;
    point.y = (pair->y << POS_SHIFT) + half;
    point.z = 0;
    return FUN_080519bc(&point);
}
