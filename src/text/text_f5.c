/* Dogru parcasi ile dikdortgen kenarlarinin kesisimi -- 0x08064724-0x0806493B
 *
 * TASLAK -- olcum sonuclari asagiya eklenecek.
 */

#include "gba_types.h"

typedef struct Point { s32 x; s32 y; } Point;
typedef struct Bounds { s32 left; s32 top; s32 pad08; s32 right; s32 bottom; } Bounds;

extern s32 FUN_0806c0f4(s32 dividend, s32 divisor);
extern s32 FUN_0800c4bc(s32 squareSum);

/* 0x08064724 */
s32 FUN_08064724(s32 steep, const Point *from, Point *hit,
                 s32 slope, s32 offset, const Bounds *box)
{
    s32 xTop, xBottom, yLeft, yRight;
    s32 flags;
    s32 best, dist, dx, dy;
    s32 left, top;

    flags = 0;

    if (steep == 0) {
        if (slope != 0) {
            xTop    = FUN_0806c0f4((box->top - offset) << 16, slope);
            xBottom = FUN_0806c0f4((box->bottom - offset) << 16, slope);
        } else {
            xBottom = 0;
            xTop = 0;
        }
        top = box->top;
        left = box->left;
        yLeft  = ((slope * left) >> 16) + offset;
        yRight = ((slope * box->right) >> 16) + offset;
    } else {
        if (slope != 0) {
            left = box->left;
            yLeft  = FUN_0806c0f4((left - offset) << 16, slope);
            yRight = FUN_0806c0f4((box->right - offset) << 16, slope);
        } else {
            yRight = 0;
            yLeft = 0;
            left = box->left;
        }
        top = box->top;
        xTop    = ((slope * top) >> 16) + offset;
        xBottom = ((slope * box->bottom) >> 16) + offset;
    }

    if (xTop > left && xTop < box->right)
        flags |= 1;
    if (xBottom > left && xBottom < box->right)
        flags |= 2;
    if (yLeft > top && yLeft < box->bottom)
        flags |= 4;
    if (yRight > top && yRight < box->bottom)
        flags |= 8;

    if (flags == 0)
        return 0;

    best = 0x7FFFFFFF;

    if (flags & 1) {
        dx = from->x - xTop;
        dy = from->y - top;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = xTop;
            hit->y = box->top;
        }
    }
    if (flags & 2) {
        dx = from->x - xBottom;
        dy = from->y - box->bottom;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = xBottom;
            hit->y = box->bottom;
        }
    }
    if (flags & 4) {
        dx = from->x - box->left;
        dy = from->y - yLeft;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = box->left;
            hit->y = yLeft;
        }
    }
    if (flags & 8) {
        dx = from->x - box->right;
        dy = from->y - yRight;
        dist = FUN_0800c4bc(dx * dx + dy * dy);
        if (dist < best) {
            best = dist;
            hit->x = box->right;
            hit->y = yRight;
        }
    }

    return best;
}
