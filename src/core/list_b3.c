/* Erase the sub-object from the cell records and mark it "done" — 0x0800DC44
 * 112 bytes.  STATUS: 104/112 bytes identical, 8 bytes differ (see below).
 *
 * WHAT IT DOES
 *   Walks the "dirty cell" list of the cell table at 0x02015650.  The list
 *   head is a count at +0x00, with entries of 4-byte (s16 x, s16 y) pairs
 *   starting at +0x04.  Each entry points at a cell in the 16x16 cell grid
 *   (a cell is 12 bytes, a row 192 bytes).  The cell's +0x00 holds the record
 *   count and its +0x08 a Sub* array; every entry in that array equal to the
 *   incoming object is cleared.  At the end, 0x400 (SUB_DONE) is added to the
 *   object's +0x0C flag word.
 *
 * WHERE THE TYPES COME FROM
 *   The grid layout was measured in the sibling src/core/listhead_e2.c (a cell
 *   is 12 bytes = count/value/data, the grid is at +0x24, width/height at
 *   +0xC24/+0xC28).  The difference here: +0x00 and +0x04..+0x23 were recorded
 *   there as "status + padding"; this function uses them as a COUNT plus an
 *   (x,y) ENTRY ARRAY (ldrsh -> signed s16, `bge`/`blt` -> a signed counter).
 *   The object carrying the +0x0C flag is the `Sub` from
 *   src/core/nodelist_a4.c; the 0x400 mask appeared there under the name
 *   SUB_DONE, and the same name was kept.
 *   0x02015650 is NOT in data/ram_map.csv; like the sibling files, it is
 *   reached through a raw address cast.  A symbol record is needed (noted in
 *   the report).
 *
 * MEASURED SOURCE-FORM LEVERS (each was tried separately)
 *
 * 1) `p` IS A SINGLE POINTER, ASSIGNED TWICE.  The ROM reads the entry fields
 *    by building a `base+6` / `base+4` pointer and adding i*4 TO IT
 *    (`adds r0,r6,#6 / adds r0,r1,r0 / movs r2,#0 / ldrsh`).  Writing
 *    `grid->touched[i].y` plainly instead builds the common subexpression
 *    `base+i*4` and puts the 6/4 into the INDEX register -- 3 instructions
 *    shorter.
 *    Separate `py`/`px` locals do not work either: agbcc moves the first out
 *    of the loop (an r8 push/pop, +16 bytes).  With a SINGLE local assigned
 *    twice, `set_in_loop != 1` and both stay inside the loop.
 *
 * 2) THE ROW OFFSET IS A SEPARATE STATEMENT, IN BYTES (`* ROWB`).  The ROM
 *    computes y*192 first, THEN reads x, makes x*12 + base and adds it last.
 *    Writing `&cells[y][x]` binds the base to the y term (fold forces the
 *    canonical `(base + y*192) + x*12`) and the y multiplication comes out
 *    AFTER the x read.  Keeping the offset in a separate statement gives the
 *    ROM's order.  `y * GW` (in cells) does not work: it produces two separate
 *    multiplications.
 *
 * 3) THE FINAL ADDITION AS AN INTEGER (`off + (s32)&cells0[x]`).  Written as
 *    pointer arithmetic, fold always moves the pointer to the LEFT and
 *    `adds r0,r0,r2` comes out; the ROM has `adds r0,r2,r0`.
 *
 * 4) `j = i * 4` IS A SEPARATE STATEMENT.  The ROM computes the index BEFORE
 *    the pointer base; written as `p[i * 2]`, the index comes after the base.
 *
 * 5) `none` IS A SEPARATE LOCAL (rule 45).  The ROM builds the zero constant
 *    BEFORE `ldr r2,[r0,#8]`; with a plain `0` in the source, agbcc moves the
 *    constant to the END of the inner loop's pre-block (two instructions swap
 *    places).
 *
 * 6) `gp` OUTSIDE THE LOOP, `grid` INSIDE IT (two separate locals).
 *    Deriving `cells0` (the grid base = grid+36) from gp makes agbcc emit
 *    `movs r0,#36 / adds r0,r0,r6` (as the ROM does); with a single local it
 *    becomes `adds r1,#36` and ONE INSTRUCTION is missing.  Because `grid` is
 *    assigned inside the loop, agbcc moves it to the pre-block and produces
 *    the ROM's `adds r6,r0,#0` register copy; the `gGrid->count` in the loop
 *    condition likewise turns into the copy `adds r7,r1,#0`.
 *
 * 7) `i = 0` IS A SEPARATE STATEMENT BEFORE THE LOOP: the ROM emits its
 *    `movs r4,#0` BEFORE the pool load.
 *
 * THE REMAINING 8 BYTES -- REGISTER ALLOCATION (an r0/r1 swap)
 *   The instruction sequence is IDENTICAL to the ROM's; only two pseudos in
 *   the guard block are allocated the other way round:
 *     ROM  : ldr r0,=0x02015650 / ldr r1,[r0] / cmp r4,r1 / bge
 *            adds r6,r0,#0 / movs r0,#36 / adds r0,r0,r6 / mov ip,r0
 *            adds r7,r1,#0
 *     ours : the same sequence with r0 and r1 swapped (that accounts for all
 *            8 bytes).
 *   MEASURED with tools/dump_alloc.py (rule 50):
 *     the pool pseudo  p24  refs 3 / lifetime 10 / priority 0.300 / order 9 -> r1
 *     the count pseudo p56  refs 3 / lifetime  6 / priority 0.500 / order 8 -> r0
 *   For the ROM's order, the pool pseudo's priority must overtake the count's:
 *   either refs must be 4 (so the floor_log2 step goes from 1 to 2) or the
 *   lifetime must drop below 6.  The callee-saved allocation (r4=i, r5=sub,
 *   r6=grid, r7=n) and ip=cells0 ARE THE SAME AS THE ROM'S -- so the problem is
 *   only the choice of a short-lived scratch register; we are at the edge of
 *   CLAUDE.md's warning that "applying the rule 50 lever there is wasted
 *   effort".
 *
 *   TRIED AND ELIMINATED THIS ROUND (do not retry)
 *     - Moving the `cells0 = gp->cells[0];` statement BEFORE `grid = gGrid;`:
 *       p24 gets refs 4 / priority 0.667 and TAKES r0 (the guard block then
 *       matches the ROM exactly), but the pre-block order inverts instead
 *       (cells0 comes BEFORE the copy) -- again 8 differences.  The two
 *       outcomes exclude each other: if cells0 comes AFTER the copy, cse2
 *       canonicalizes the operand to `grid` (p23) and kills p24's fourth
 *       reference.
 *     - Reading the `.x` field through `gGrid->touched[0].x`: the allocation
 *       becomes EXACTLY the ROM's (40 bytes overlapping), but the address
 *       folds into a second pool word -> 116 bytes.
 *     - 11 declaration rotations, 6 permutations of the pre-block statements x
 *       2 starting orders, `while`/`for`/an explicit `do-while` (92
 *       differences), the three in-body positions of `i++`, 5 forms of the
 *       loop bound (`gp->count` 124 bytes, `*(long *)&...` 8 differences,
 *       `+0`, `(s32)`), gp as a `u8 *`, 27 combinations of the gp/grid roles,
 *       two separate pointers instead of `p` (128 bytes), the inline form of
 *       the address reads (116 bytes), moving cells0 out of the loop (14
 *       differences), and the extern symbol form (probed with another symbol
 *       from ram_map -- BYTE-IDENTICAL code to the raw cast).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/list_b3.c
 */

#include "gba_types.h"

/* A grid row: 16 cells x 12 bytes. */
#define GW              16
#define ROWB            (GW * 12)

/* The sub-object's +0x0C flag (the same name as in src/core/nodelist_a4.c). */
#define SUB_DONE        0x400

/* 0x02015650 is NOT in data/ram_map.csv; reached through a raw address cast. */
#define gGrid           ((Grid *)0x02015650)

/* The object held in the records; only the flag field is used here. */
typedef struct Sub {
    u8  pad00[12];
    u32 flags;                  /* +0x0C */
} Sub;

/* A grid cell (the layout was measured in src/core/listhead_e2.c). */
typedef struct Cell {
    s32   count;                /* +0x00 record count */
    u32   value;                /* +0x04 */
    Sub **data;                 /* +0x08 record array */
} Cell;

/* A dirty-cell entry: a grid coordinate. */
typedef struct Touched {
    s16 x;                      /* +0x00 column (12-byte stride) */
    s16 y;                      /* +0x02 row (192-byte stride) */
} Touched;

typedef struct Grid {
    s32     count;              /* +0x0000 dirty cell count */
    Touched touched[8];         /* +0x0004 */
    Cell    cells[GW][GW];      /* +0x0024 */
    s32     width;              /* +0x0C24 */
    s32     height;             /* +0x0C28 */
} Grid;

/* 0x0800DC44 */
void FUN_0800dc44(Sub *sub)
{
    Grid *grid;
    Grid *gp;
    Cell *cells0;
    Cell *cell;
    Sub **entry;
    Sub  *none;
    s16  *p;
    s32   i;
    s32   j;
    s32   k;
    s32   off;

    i = 0;
    gp = gGrid;
    for (; i < gGrid->count; i++) {
        grid = gGrid;
        cells0 = gp->cells[0];

        /* Entry fields: i*4 is added to a base+6 / base+4 pointer.
         * `p` is assigned twice DELIBERATELY (header note 1). */
        j = i * 4;
        p = &grid->touched[0].y;
        off = *(s16 *)((u8 *)p + j) * ROWB;
        p = &grid->touched[0].x;
        cell = (Cell *)(off + (s32)&cells0[*(s16 *)((u8 *)p + j)]);

        k = cell->count;
        if (k > 0) {
            none = 0;
            entry = cell->data;
            do {
                if (*entry == sub)
                    *entry = none;
                entry++;
                k--;
            } while (k != 0);
        }
    }

    sub->flags |= SUB_DONE;
}
