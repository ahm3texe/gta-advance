/* Testing the slot's four points against the map and producing entries
 *      0x080651E0-0x08065377, 408 bytes
 *
 * SpawnSlotEffect (0x08065130) calls this as `PlaceProbeEntries(context, 1)`;
 * the second argument is UNUSED (the ROM's first act is to clobber r1 with
 * `mov r1, sp`), and the return value is the number of entries produced.
 *
 * The context holds four points at +0x74 with a 12-byte stride.  Each point's
 * x/y is shifted right by 22 bits (10.22 fixed point) to become a map cell.
 * The first loop CLAMPS the coordinates to the map bounds and, if the field in
 * bits 7..9 of the cell is 4, produces an entry with phase 52 and bumps the
 * counter; otherwise it sets the `blocked` flag.  If the flag was set, only
 * trigger number 471 is fired.  If it was not, the second loop repeats the
 * same test with rejection INSTEAD OF clamping (an out-of-bounds point is
 * skipped) and produces entries with phase 51; if more than four qualify,
 * 0x02 is added to the owner's +0x0A flag, then one more entry for the
 * context's own end and trigger number 470 follow.
 *
 * The ROM table gRom08852A1C = { -128, 128, -384, 384 }: 16 bytes copied into
 * a local array in a single block (`ldmia/stmia` + `ldr/str`), so the source
 * has a local aggregate assignment.  The id is derived as
 * `(id + off.v[i]) & 0x3FF`.
 *
 * Four measured rules (all proved in this function):
 *
 * 1. The cell value MUST be taken into a `u32` local.  Writing
 *    `(*p & 0x380) >> 7` directly keeps the expression in HImode: an extra
 *    `movs #0xe0 / lsls #2 / adds r0,r2,#0` copy plus 16/23 shift pairs, i.e.
 *    4 extra RTL instructions per loop.  An `s32` local diverges by 2 bytes.
 * 2. Those 4 instructions also decide the loop's counting direction: agbcc's
 *    `check_dbra_loop` pass wants `lifetime * 15 * benefit >= instruction
 *    count` in order to reduce givs; the `off.v[i]` giv's benefit is 4
 *    (1*15*4 = 60), and because the loop is 63 instructions in the HImode
 *    spelling the giv is not reduced, the biv does not die, and the loop
 *    counts up instead of using `movs #3 / negs` (loop dump: "giv of insn 170
 *    not worth while, 60 vs 63").
 * 3. `gRam0201AEE8` cannot be taken into a SHARED local pointer across the two
 *    loops: a shared variable raises the reference count to 16, lifts the
 *    priority to 3.2 and takes r0 for the pointer (the ROM: r3 in the first
 *    loop, r2 in the second).  Direct global access -- or a SEPARATE local per
 *    loop -- gives the right allocation (the same lever as rule 59's
 *    three-separate-pointers note).
 * 4. The row offset must be taken into a SEPARATE local:
 *      row = y * gRam0201AEE8->width;  tile = *(tiles + x + row);
 *    Written as a single expression, `... + y * gRam...->width` binds the
 *    `muls` destination to y's register (`muls r1,r0`); the ROM puts the
 *    product in the WIDTH's register with `muls r0,r1`.  Reversing the
 *    operands (`width * y`) fixes the destination but produces an extra load
 *    pseudo and breaks the width/pointer/y allocation (23 bytes off).  The
 *    intermediate variable gives both at once.
 *
 * The summed address expression `*(tiles + x + row)` produces two SEPARATE
 * scalings (the ROM: `lsls #1` twice, separate `adds`); the array spelling
 * `tiles[x + row]` does a single scaling and diverges by 4 bytes.
 *
 * Spellings ruled out: the `tiles[y*w + x]` array index; the `tiles + y*w + x`
 * and `tiles + w*y + x` addition orders (34 bytes); `row = width * y` (171);
 * `cell = tiles + x; cell[y*w]` (the sizes do not work out); the cell value as
 * `s32` (11); `w`/`h` locals for the width and height (23); a shared `grid`
 * local pointer (23); 72 permutations of the declaration order (none of them
 * has any effect).
 *
 * MATCH: 408/408 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_probe.c
 */


#include "gba_types.h"
#include "map_grid.h"

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

typedef struct Owner {
    u8  pad00[10];
    u16 flags;                  /* +0x0A */
} Owner;

typedef struct Probe {
    Triple head;                /* +0x00, the source triple in the last call */
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

extern void CreateEntry(Triple *src, u32 arg1, u32 phase, u32 owner);
extern void FUN_08035168(s32 sound);

/* 0x080651E0 */
s32 PlaceProbeEntries(Probe *probe, s32 mode)
{
    Offsets off;
    s32     count;
    s32     blocked;
    s32     id;
    s32     placed;
    s32     i;
    s32     x;
    s32     y;
    u32     tile;
    s32     row;

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
        if (x >= gRam0201AEE8->width)
            x = gRam0201AEE8->width - 1;
        if (y >= gRam0201AEE8->height)
            y = gRam0201AEE8->height - 1;
        row = y * gRam0201AEE8->width;
        tile = *(gRam0201AEE8->tiles + x + row);
        if (((tile & TILE_FIELD) >> TILE_SHIFT) == TILE_WANTED) {
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
                if (x < gRam0201AEE8->width && y < gRam0201AEE8->height) {
                    row = y * gRam0201AEE8->width;
                    tile = *(gRam0201AEE8->tiles + x + row);
                    if (((tile & TILE_FIELD) >> TILE_SHIFT) == TILE_WANTED) {
                        CreateEntry(&probe->spots[i], id, PHASE_SPAWN,
                                    (u32)probe->owner);
                        placed++;
                    }
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
