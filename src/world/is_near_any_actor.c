/* Konum herhangi bir aktore yakin mi — 0x08054F1C-0x08055051
 *
 * Maskenin 14/12/128 bitlerine gore uc listeyi (0x0202F2C0, 0x0202F310,
 * 0x02028290) geziyor; +0x0C'de 0x40 kurulu olmayan her aktorun +0x18
 * konumuna Manhattan benzeri yaklasik uzaklik (|dx|+|dy| - m/2 - m/4 +
 * m/16, m = min) hesaplaniyor. Ilk listede uzaklik <= 0x800000 (isaretsiz),
 * diger ikisinde <= 0x3FFFFF (isaretli) ise 0 doner; hicbiri yakin degilse
 * 1. Ilk dongudeki `best` en kucugu tutuyor ama hic kullanilmiyor -- ROM'da
 * da oyle (r4), kaynakta kalintisi var.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_near_any_actor.c
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
