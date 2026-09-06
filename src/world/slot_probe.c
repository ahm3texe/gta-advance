/* Yuva noktalarini haritada sinama ve giris uretme — 0x080651E0-0x08065377
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_probe.c
 */

#include "gba_types.h"

#define SPOT_COUNT     4
#define TILE_FIELD     0x380        /* bit 7..9 */
#define TILE_SHIFT     7
#define TILE_WANTED    4
#define ID_MASK        0x3FF
#define PHASE_PLACE    52
#define PHASE_SPAWN    51
#define OWNER_BIT      2
#define SOUND_OK       470
#define SOUND_FAIL     471
#define CROWD_LIMIT    3

typedef struct Triple {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Triple;

typedef struct Grid {
    u16  width;                 /* +0x00 */
    u16  height;                /* +0x02 */
    u16 *tiles;                 /* +0x04 */
} Grid;

typedef struct Owner {
    u8  pad00[10];
    u16 flags;                  /* +0x0A */
} Owner;

typedef struct Probe {
    Triple head;                /* +0x00, son cagrida kaynak uclu */
    u8     pad0c[2];
    s16    id;                  /* +0x0E */
    u8     pad10[8];
    u32    ready;               /* +0x18 */
    u8     pad1c[0x64 - 0x1C];
    Owner *owner;               /* +0x64 */
    u8     pad68[0x74 - 0x68];
    Triple spots[SPOT_COUNT];   /* +0x74 */
} Probe;

typedef struct Offsets {
    s32 v[SPOT_COUNT];
} Offsets;

extern const Offsets gRom08852A1C;
extern Grid *gRam0201AEE8;

extern void CreateEntry(Triple *src, u32 arg1, u32 phase, u32 owner);
extern void FUN_08035168(s32 sound);

/* 0x080651E0 */
s32 FUN_080651e0(Probe *probe, s32 mode)
{
    Offsets off;
    Grid   *grid;
    s32     count;
    s32     blocked;
    s32     id;
    s32     placed;
    s32     i;
    s32     x;
    s32     y;

    count = 0;
    blocked = 0;
    off = gRom08852A1C;

    if (probe->ready == 0)
        return 0;

    id = probe->id;

    for (i = 0; i < SPOT_COUNT; i++) {
        x = probe->spots[i].x >> 22;
        y = probe->spots[i].y >> 22;
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        grid = gRam0201AEE8;
        if (x >= grid->width)
            x = grid->width - 1;
        if (y >= grid->height)
            y = grid->height - 1;
        if (((grid->tiles[y * grid->width + x] & TILE_FIELD) >> TILE_SHIFT)
            == TILE_WANTED) {
            CreateEntry(&probe->spots[i], (id + off.v[i]) & ID_MASK,
                        PHASE_PLACE, (u32)probe->owner);
            count++;
        } else {
            blocked = 1;
        }
    }

    if (blocked == 0) {
        placed = 0;
        for (i = 0; i < SPOT_COUNT; i++) {
            x = probe->spots[i].x >> 22;
            y = probe->spots[i].y >> 22;
            if (x >= 0 && y >= 0) {
                grid = gRam0201AEE8;
                if (x < grid->width && y < grid->height
                    && ((grid->tiles[y * grid->width + x] & TILE_FIELD)
                        >> TILE_SHIFT) == TILE_WANTED) {
                    CreateEntry(&probe->spots[i], id, PHASE_SPAWN,
                                (u32)probe->owner);
                    placed++;
                }
            }
        }
        if (placed > CROWD_LIMIT)
            probe->owner->flags |= OWNER_BIT;
        CreateEntry(&probe->head, id, PHASE_SPAWN, (u32)probe->owner);
        FUN_08035168(SOUND_OK);
    } else {
        FUN_08035168(SOUND_FAIL);
    }

    return count;
}
