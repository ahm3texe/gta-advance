/* Updates the BG2 affine matrix and the 24.8 reference coordinates.
 * PARKED: contiguous fields, separate offset variables and MMIO pointer
 * increments were tried. The best output is 168/176 bytes; the remaining
 * obstacles are the r8/r9 live ranges and the two work-area offsets being
 * placed separately in the literal pool. */

#include "gba_types.h"

extern s32 gBg2ScaleX;
extern s32 gBg2ScaleY;
extern u8 gRam0201AEF0[];
extern volatile s16 gRegBg2XLow;
extern volatile s16 gRegBg2XHigh;
extern volatile s16 gRegBg2YHigh;

/* 0x08011DD4 */
void FUN_08011dd4(s32 x, s32 y, s32 center)
{
    s32 scaledX;
    s32 scaledY;
    s32 screenX;
    s32 screenY;
    s32 refX;
    s32 refY;
    u32 screenXOffset;
    u32 screenYOffset;
    volatile s16 *high;
    volatile s16 *matrix;

    x <<= 16;
    y <<= 16;
    scaledX = gBg2ScaleX * 40;
    screenX = scaledX >> 6;
    scaledY = gBg2ScaleY * 40;
    screenY = scaledY >> 6;
    screenXOffset = 0x1F88;
    *(s32 *)(gRam0201AEF0 + screenXOffset) = screenX;
    screenYOffset = 0x1F8C;
    *(s32 *)(gRam0201AEF0 + screenYOffset) = screenY;

    center = (center << 16) >> 1;
    refX = center + x - screenX * 120;
    refY = center + y - screenY * 120;

    gRegBg2XLow = refX >> 8;
    high = &gRegBg2XHigh;
    *high++ = refX >> 24;
    *high = refY >> 8;
    matrix = &gRegBg2YHigh;
    *matrix = refY >> 24;

    matrix -= 7;
    *matrix++ = scaledX >> 14;
    *matrix++ = 0;
    *matrix++ = 0;
    *matrix = scaledY >> 14;
}
