/* Try the position shifted towards the target; if that fails, scan the
 * neighbouring tiles
 * 0x08055054-0x0805518B  (312 bytes; the last 4 are a literal pool)
 *
 * The call receives a position pointer (in/out) and a (dx, dy) shift. The
 * point `position + (dx,dy)<<16` is first tested with IsNearAnyActor; if it is
 * free, the position is moved there and 1 is returned. Otherwise a box of
 * +-128.0 (0x800000) is built around the requested point and passed to
 * FUN_08040700, which writes the (tx, ty) indices of at most 8 tiles falling
 * inside the box. For each tile the tile centre (tx<<22 + dx<<16 + 32.0) is
 * tested again; at the first free tile the position's x/y are pulled there and
 * 1 is returned. If none works, 0.
 *
 * The scan runs at most TWO rounds: in the first (only when useTileMask), the
 * mask of the tile kind the player is standing on (1 << GetTileFieldA2); in
 * the second, the mask supplied by the caller. If the two masks give the same
 * result, or if useTileMask is zero, a single round is enough.
 *
 * STRUCTURE HINTS:
 *   - The position is a 12-byte {s32 x, y, z}: on the first success it is
 *     copied in one go with `ldmia/stmia {r2,r3,r4}` (rule 32 -- struct
 *     assignment).
 *   - The box is two separate Vec3s: FUN_08040700 reads it as [r0,#0]/[r0,#4]
 *     and [r0,#12]/[r0,#16], i.e. an array of 2 with a 12-byte stride.
 *   - The tile buffer is 8 x {u16 tx, u16 ty} = 32 bytes (sp+40..71).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_b7.c
 *
 * FOUR MEASURED DETAILS (each was tried on its own; all four are decisive):
 *
 * 1) THE SHIFT CONSTANT (dx<<16)+32.0 MUST BE A LOCAL IN THE INNER LOOP BODY.
 *    The ROM moved it into the inner loop's pre-header (AFTER the guard):
 *    that is, it is written INSIDE the inner loop in the source and
 *    loop-invariant motion lifts it out. Writing it BEFORE the inner loop (in
 *    the outer loop body) lets agbcc lift it out of the OUTER loop too and
 *    spread it across two extra stack slots (0x68/0x6c): the frame grows
 *    108 -> 116, 308 bytes.
 *    Writing the expression inline moves nothing at all: fold turns
 *    `x + (y + CONST)` into `(x + CONST) + y` and leaves the dx load inside
 *    the loop (300 bytes).
 *
 * 2) THE TWO FIELDS WANT TWO DIFFERENT ADDRESSING FORMS. The ROM reads tx with
 *    a walking pointer (`ldrh r2,[r4,#0]` + `adds r4,#4`) and ty with an
 *    index.
 *    Writing `cells[i].tx` / `cells[i].ty` triggers agbcc's combine_givs: a
 *    single pointer plus `[r4,#2]` comes out (6 bytes short, 304).
 *    Writing both through the pointer (`cell->tx`, `cell->ty`) produces TWO
 *    separate walking pointers (308). The solution: the tx field access stays,
 *    while ty is read through a SEPARATE view, `((u16 *)cells)[i * 2 + 1]` --
 *    that form is not combined as a giv and reproduces the ROM's index
 *    computation exactly. 304 -> 312 bytes.
 *
 * 3) DECLARATION ORDER DECIDES THE STACK SLOTS. When the frame overflows,
 *    reload hands out slots in PSEUDO NUMBER order, and the pseudo number
 *    comes from DECLARATION order (expand_decl). In the ROM fallback is at
 *    0x5c and tileMask at 0x60. With the declarations reversed the slots were
 *    swapped.
 *    By the same mechanism, moving the `yoff` declaration AFTER `count` put
 *    ybase in r3 instead of r7 and count in r7 instead of r3 (19 -> 9
 *    differences).
 *    138 permutations of the 9 declaration names were swept; this ordering is
 *    the best.
 *
 * 4) `pp = &probe;` MUST BE THE FIRST STATEMENT OF THE INNER LOOP. In the
 *    ROM's pre-header `add r5,sp,#72` comes FIRST; that is, &probe is taken
 *    into a local at the top of the loop in the source and moved out first in
 *    the movable order. Using the address only as a call argument (`&probe`)
 *    puts it at the END of the movable list and breaks the pre-header order.
 *
 * OTHER RULES APPLIED:
 *   - Rule 9/31: `attempt`, `count` and `i` are signed `int`; the ROM emits
 *     `bge`/`blt` and `ble` (signed).
 *   - Rule 32: on the first success `*pos = want;` is a struct assignment ->
 *     `ldmia/stmia {r2,r3,r4}`.
 *   - Rule 35: the final `pop {r1}; bx r1` with r0 live -> a u32 return.
 *   - The parameters are s32/u32; a narrow type would add an lsls/lsrs
 *     normalization at entry (rule 15), and the ROM has none.
 *
 * ELIMINATED PATHS (DO NOT RETRY):
 *   - `cells[i].ty` (giv combination, 304) and `cell->ty` (a second pointer,
 *     308); `*(u16 *)((u8 *)cells + i*4 + 2)` (308); a `volatile` ty (308).
 *   - Computing xoff/yoff BEFORE the inner loop (308) or writing them inline
 *     (300).
 *   - The `for (i = 0, cell = cells; ...; i++, cell++)` form (308).
 *   - Declaration order: 138 permutations were swept; the current order is the
 *     best.
 *   - Everything tried for the last 2-byte difference (`adds r3,r4,#0`), NONE
 *     of which worked (all >= 9 differences): nested `if`s, `if/else`, a
 *     ternary, an inverted condition (`sel = tileMask` + `||`), a labelled
 *     `goto have_sel` form, making `sel` int/s32/u32, a second local
 *     `arg = sel;` (rule 17), making `count` u32, taking the `MAX_CELLS`/`box`/
 *     `cells` arguments into separate locals (rule 18), the `&box[0]` form,
 *     using the `mask` parameter directly instead of `fallback` (308),
 *     assigning `fallback` at the top of the function (308), moving
 *     `fallback = mask` inside the loop (20), a `zero` local for the fifth
 *     argument (13), taking `probeMask` into a local (14), assigning `sel`
 *     before the loop and updating it inside (carry: 316/34 and 312/9),
 *     writing the call twice in an if/else (320), selecting through a
 *     `u32 *selp` pointer (320), and using `sel == fallback` in the bottom
 *     test (46). The `agbcc` compiler variant was tried too: same result.
 *
 * THE REMAINING DIFFERENCE -- THE MECHANISM WAS MEASURED, THERE IS NO LEVER
 *   tools/dump_alloc.py: the `sel` pseudo (p31, 6 refs / 12 lifetime, priority
 *   1.000, allocation order 3, calls_crossed = 0) falls to r3. In agbcc's
 *   global.c, `find_reg` also considers r0..r3 as candidates for an allocno
 *   that does not cross a call; because r0/r1/r2 clash with the argument
 *   setup, the first free one, r3, is chosen. In the ROM the same value is in
 *   r4, so there r0..r3 are NOT candidates -- which only happens when
 *   `calls_crossed != 0` (the `call_used_reg_set` is then excluded and the
 *   first free callee-saved register, r4, remains).
 *   So in the ROM's source `sel`'s lifetime CROSSES the FUN_08040700 call.
 *   There is no other instruction between the call and the inner loop in the
 *   ROM; every semantically equivalent form that lengthens the lifetime (the
 *   carry/guard/selp attempts above) also makes `sel` clash with r4 (the tile
 *   pointer) in the inner loop and spills it.
 *   No other lever was found at source level.
 *
 * STATUS: 312/312 bytes -- the size is EXACT; 142/151 instructions identical;
 * of the register operand counts only r3 (22 vs 23), r4 (18 vs 15) and r0
 * (74 vs 76) deviate -- all three stem from the same single difference (the
 * missing `adds r3,r4,#0` and the 2 bytes of alignment padding that replace
 * it). The remaining 8 instruction differences are the branch offsets shifted
 * by those 2 bytes, not a separate defect.
 */

#include "gba_types.h"

#define BOX_RADIUS      0x00800000      /* 128.0, 16.16 fixed-point */
#define HALF_TILE       0x00200000      /*  32.0 = half a tile      */
#define PROBE_LIMIT     0x00180000      /*  24.0; IsNearAnyActor threshold */
#define MAX_CELLS       8
#define TILE_SHIFT      22              /* tile index -> world coordinate */
#define POS_SHIFT       16              /* integer -> 16.16          */

/* A world position; the z field is carried along but not used in the test. */
typedef struct Vec3 {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Vec3;

/* The tile index pair written by FUN_08040700. */
typedef struct Cell {
    u16 tx;                     /* +0x00 */
    u16 ty;                     /* +0x02 */
} Cell;

extern u32 GetTileFieldA2(const Vec3 *pos);
extern u32 IsNearAnyActor(const Vec3 *pos, u32 mask, u32 limit);
extern int FUN_08040700(const Vec3 *box, Cell *out, int max, u32 mask, u32 opt);

/* 0x08055054 */
u32 FUN_08055054(Vec3 *pos, u32 mask, u32 useTileMask, u32 probeMask,
                 s32 dx, s32 dy)
{
    Vec3 want;
    Vec3 box[2];
    Cell cells[MAX_CELLS];
    Vec3 probe;
    Vec3 *pp;
    u32  fallback;
    u32  tileMask;
    u32  sel;
    s32  xoff;
    int  attempt;
    int  count;
    s32  yoff;
    int  i;

    want.x = pos->x + (dx << POS_SHIFT);
    want.y = pos->y + (dy << POS_SHIFT);
    want.z = pos->z;
    tileMask = 1 << GetTileFieldA2(pos);

    if (useTileMask != 0) {
        if (IsNearAnyActor(&want, probeMask, PROBE_LIMIT)) {
            *pos = want;
            return 1;
        }
    }

    box[0].x = want.x - BOX_RADIUS;
    box[1].x = want.x + BOX_RADIUS;
    box[0].y = want.y - BOX_RADIUS;
    box[1].y = want.y + BOX_RADIUS;

    fallback = mask;
    for (attempt = 0; attempt <= 1; attempt++) {
        sel = fallback;
        if (useTileMask != 0 && attempt == 0)
            sel = tileMask;

        count = FUN_08040700(box, cells, MAX_CELLS, sel, 0);
        for (i = 0; i < count; i++) {
            pp = &probe;
            xoff = (dx << POS_SHIFT) + HALF_TILE;
            pp->x = (cells[i].tx << TILE_SHIFT) + xoff;
            yoff = (dy << POS_SHIFT) + HALF_TILE;
            pp->y = (((u16 *)cells)[i * 2 + 1] << TILE_SHIFT) + yoff;
            pp->z = 0;
            if (IsNearAnyActor(pp, probeMask, PROBE_LIMIT)) {
                pos->x = pp->x;
                pos->y = pp->y;
                return 1;
            }
        }

        if (useTileMask == 0)
            break;
        if (tileMask == fallback)
            break;
    }

    return 0;
}
