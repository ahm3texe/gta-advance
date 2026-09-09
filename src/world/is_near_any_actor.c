/* Test proximity to any actor — 0x08054F1C-0x08055051
 *
 * Mask bits 14/12/128 select three lists (0x0202F2C0, 0x0202F310, 0x02028290).
 * For each actor without bit 0x40 at +0x0C, compute an approximate distance
 * to its position at +0x18: |dx|+|dy| - m/2 - m/4 + m/16, where m is the
 * minimum. Return 0 for a distance <= 0x800000 (unsigned) in the first list
 * or <= 0x3FFFFF (signed) in the other two; return 1 if none is near.
 * The first loop tracks an unused minimum in best, just as the ROM does
 * in r4; the source retains it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/is_near_any_actor.c
 */

#include "gba_types.h"
#define FLAG_SKIP 0x40
typedef struct Pos { s32 x; s32 y; } Pos;
typedef struct Actor { struct Actor *next; u8 pad04[8]; u32 flags; u8 pad10[8]; Pos *pos; } Actor;
extern Actor *GetUnk0202F2C0(void);
extern Actor *GetUnk0202F310(void);
extern Actor *GetUnk02028290(void);
u32 IsNearAnyActor(Pos *pos, u32 mask)
{
    s32 x; s32 y; u32 best; Actor *a; Pos *p; s32 dx; s32 dy; s32 m; s32 d;
    x = pos->x;
    y = pos->y;
    best = 0x7FFFFFFF;
    if (mask & 14) {
        for (a = GetUnk0202F2C0(); a != 0; a = a->next) {
            if (a->flags & FLAG_SKIP)
                continue;
            p = a->pos;
            dx = x - p->x;
            if (dx < 0) dx = -dx;
            dy = y - p->y;
            if (dy < 0) dy = -dy;
            m = dy;
            if (m > dx) m = dx;
            d = dx + dy - (m >> 1) - (m >> 2) + (m >> 4);
            if (d < 0) d = -d;
            if ((u32)d <= 0x800000)
                return 0;
            if ((u32)d < best)
                best = d;
        }
    }
    if (mask & 12) {
        for (a = GetUnk0202F310(); a != 0; a = a->next) {
            if (a->flags & FLAG_SKIP)
                continue;
            p = a->pos;
            dx = x - p->x;
            if (dx < 0) dx = -dx;
            dy = y - p->y;
            if (dy < 0) dy = -dy;
            m = dy;
            if (m > dx) m = dx;
            d = dx + dy - (m >> 1) - (m >> 2) + (m >> 4);
            if (d < 0) d = -d;
            if (d <= 0x3FFFFF)
                return 0;
        }
    }
    if (mask & 128) {
        for (a = GetUnk02028290(); a != 0; a = a->next) {
            if (a->flags & FLAG_SKIP)
                continue;
            p = a->pos;
            dx = x - p->x;
            if (dx < 0) dx = -dx;
            dy = y - p->y;
            if (dy < 0) dy = -dy;
            m = dy;
            if (m > dx) m = dx;
            d = dx + dy - (m >> 1) - (m >> 2) + (m >> 4);
            if (d < 0) d = -d;
            if (d <= 0x3FFFFF)
                return 0;
        }
    }
    return 1;
}
