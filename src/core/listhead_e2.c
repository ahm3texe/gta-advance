/* Build the cell table and partition the arena — 0x0800CB08-0x0800CC63 (348 bytes)
 *
 * PARKED: 291/348 bytes differ (the remaining difference is entirely REGISTER
 * ALLOCATION; 103/173 of the instruction sequence is identical). The detailed
 * elimination record is below.
 *
 *   1) DMA3 zeroes 3 words at 0x0201AA80 (a small block), inside a
 *      save/restore pair for REG_IME.
 *   2) The `>> 10` shifted forms of the two incoming coordinates are written
 *      into the +0xC24 / +0xC28 fields of the table at 0x02015650 (asrs =
 *      SIGNED); these are the bounds of the two loops below. The fourth
 *      parameter (0/1/2) selects one of three ROM tables and writes it to
 *      0x0201AA98; all three share a single `str` (agbcc cross-jumping, the
 *      inverse direction of rule 29).
 *   3) DMA3 zeroes 0x11F8 words at 0x02016290 (a large block). That block is
 *      partitioned as an ARENA: each cell takes as many words as its own
 *      `count`, and the `data` pointer advances by count*4.
 *   4) At the end the table status word, gListHead02016280 and the 0x0201AA9C
 *      flag are cleared.
 *
 * MEASURED LAYOUT (from the disassembly)
 *   source stream (param0)  8 bytes:  +0x00 u16 count, +0x04 u32 value
 *   cell                   12 bytes:  +0x00 count, +0x04 value, +0x08 arena
 *   table (0x02015650)               +0x00 status, +0x24 16x16 cells,
 *                                    +0xC24 width, +0xC28 height
 *   0x24 + 16*16*12 = 0xC24 fits exactly, with no intermediate padding.
 *   The outer loop i (stride 12 bytes) runs against width and the inner loop j
 *   (stride 192 bytes = 16*12) against height; the source stream advances
 *   CONTINUOUSLY across both.
 *
 * ADDRESSING: the base is loaded PLAINLY in the ROM and the offsets are built
 * as SEPARATE literals (`ldr r0,=0x02015650 / ldr r4,=0xc24 / adds r3,r0,r4`).
 * One might think this requires an extern symbol; it was MEASURED, and it does
 * not: agbcc produces the SAME form from a constant cast and from a real
 * extern in ram_map (probe: both forms give `ldr base / ldr offset / add`).
 * So rule 1's folding trap does not arise here. None of the five addresses
 * (0x02015650, 0x02016290, 0x0201AA80, 0x0201AA98, 0x0201AA9C) is in
 * data/ram_map.csv; the sibling file src/core/list_b1.c writes three of them
 * in the same form.
 *
 * FOUR MEASURED DETAILS (each was tried on its own)
 *
 * 1) THE BASE MUST BE A SEPARATE LOCAL, but the FIRST TWO WRITES must come
 *    FROM THE CONSTANT CAST.
 *    Writing `grid = gGrid02015650;` up front and using `grid->` everywhere
 *    ties the base and the first two writes into one pseudo and moves the
 *    `mov ip,base` instruction FORWARD; in the ROM the copy comes AFTER the
 *    two writes (`mov r8,r0`). Writing the two stores through the constant
 *    cast and everything after them through the local gives the ROM's order
 *    (CSE already merges the base into a single pseudo within the block).
 *    Writing everything through the constant cast, on the other hand, DROPS
 *    the base in the loop: the in-loop accesses turn into FOLDED literals such
 *    as `.word 0x2016274` (measured: 308/352 differences, and the size does
 *    not match either).
 *
 * 2) `gListHead02016280` IS WRITTEN AS A RAW ADDRESS, NOT as an extern symbol
 *    -- a deliberate and MEASURED deviation. agbcc hoists a SYMBOL_REF into a
 *    register BEFORE the loop (`ldr r0,=symbol / mov r9,r0`) and holds that
 *    register throughout the loop; the ROM loads the address AT THE VERY END.
 *    Measurement: the extern form gives 360 bytes / 320 differences, the raw
 *    address form 348 bytes / 291 differences. A CONST_INT is not hoisted the
 *    same way.
 *    (See the docs/COMPILER.md note "rule 1 is not universal".)
 *    The name is kept in this comment so a rename can still find it.
 *
 * 3) THE INNER LOOP BOUND: in the ROM the guard value is read ONCE BEFORE the
 *    outer loop and kept in `ip`, but the condition BELOW the loop is RE-READ
 *    from memory on every iteration (`ldr r0,[r6]`). For two copies of the
 *    same expression that can only happen if the ALIAS SETS differ: an `s32`
 *    read and the `u32` writes to the cell are in the same alias set, so agbcc
 *    cannot hoist the guard. Reading the guard through `*(long *)` -- `long`
 *    and `int` are SEPARATE alias sets in C and, unlike a signed/unsigned
 *    pair, do not merge -- makes the guard hoistable while the plain read
 *    below the loop stays unhoistable. That is exactly the ROM's pattern.
 *    Measurement: an `s32` guard gives 352 bytes/313 differences, the `long`
 *    view 348 bytes/291 differences.
 *    This is the alias-set counterpart of rule 39 ("apply the qualifier to
 *    THAT ACCESS, not to the whole field").
 *
 * 4) The outer loop is a `for`, the inner one an EXPLICIT `do/while`
 *    (rule 40). Writing the inner loop as a `for` as well ties the guard and
 *    the bottom condition into a single expression and makes the distinction
 *    in point 3 impossible (measured: 340 bytes, 8 bytes SHORTER than the
 *    ROM).
 *
 *   - All accesses through the constant cast (`gGrid02015650->...`): 352/308,
 *     the in-loop addresses fold and the base is not held in a register.
 *   - Separate objects instead of `Grid` (such as `(s32*)0x02016274`): the
 *     base+offset form disappears; eliminated.
 *   - An extern symbol base (probed with a real symbol from ram_map):
 *     BYTE-IDENTICAL code to the constant cast. So the missing symbol is NOT
 *     an obstacle.
 *   - Caching `height` in a local and using it both in the guard and in the
 *     bottom condition: 340 bytes (8 SHORT), the read in the bottom condition
 *     disappears.
 *   - Reading the bottom condition through a `volatile` view: it turns on
 *     `loop_has_volatile`, which also kills the address hoists, giving 340
 *     bytes -- worse.
 *   - `long` only on the struct field / on both width and height / only on the
 *     guard: all three give the same 291; the most readable one (the cast on
 *     the guard alone) was chosen.
 *   - Permutations of the source order (the order of the `cells`/`src`/`width`
 *     assignments): 291 / 291 / 295. The best was kept.
 *   - Without the `cells` local (`&grid->cells[j][i]` directly): 340 bytes;
 *     agbcc folds the +0x24 into i*12, while the ROM keeps `grid+36` as a
 *     separate loop invariant.
 *
 * THE SOURCE OF THE REMAINING DIFFERENCE
 *   THE FIRST DIFFERENCE IS AT +0x00A (0x0800CB12): the ROM has `sub sp,#8`,
 *   ours `sub sp,#4` -- so the ROM has ONE more stack slot right from the
 *   start (a width spill). The shape largely fell into place; what remains is
 *   ALLOCATION. ROM: base r8, cell base r9, &height sl, the height value ip,
 *   width ON THE STACK (`str r3,[sp,#4]`, hence `sub sp,#8`). Ours: base ip,
 *   cell base r9, width sl, height r8, &height recomputed in the inner
 *   pre-block, `sub sp,#4`.
 *   So the ROM makes ONE spill that we do not. Examined with
 *   tools/dump_alloc.py: the base pseudo has 19 references / 110 lifetime ->
 *   priority 0.691; it is allocated LAST among the high registers, whereas in
 *   the ROM it is FIRST. Flipping the priority requires changing the base's
 *   reference/lifetime ratio; that was not found this round.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 * Rule 3:  the stack temporary is `volatile`, otherwise the second zero store
 *          is eliminated.
 * Rule 42/9: the loop counters are SIGNED (`s32`) -- the ROM emits
 *            `bge`/`blt`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/listhead_e2.c
 */

#include "gba_io.h"

#define DMA_FILL_BIG    0x850011F8
#define DMA_FILL_SMALL  0x85000003

/* Row width of the cell table: 192 bytes / 12 bytes = 16. */
#define GRID_WIDTH      16

/* None of these is in data/ram_map.csv; they are reached through raw address
 * casts. gListHead02016280 IS in the map, but a raw address is used here
 * deliberately (header comment, point 2). */
#define gGrid02015650    ((Grid *)0x02015650)
#define gArena02016290   ((u32 *)0x02016290)
#define gBuf0201AA80     ((void *)0x0201AA80)
#define gTable0201AA98   (*(const void **)0x0201AA98)
#define gListHead02016280 (*(void **)0x02016280)   /* ram_map: gListHead02016280 */
#define gFlag0201AA9C    (*(u8 *)0x0201AA9C)

/* The three mode tables are in ROM; rule 1 applies to RAM only. */
#define TABLE_MODE_0    ((const void *)0x08EBFBA4)
#define TABLE_MODE_1    ((const void *)0x08EC14B4)
#define TABLE_MODE_2    ((const void *)0x08EC2DC4)

/* An entry of the source stream: 8 bytes; the second halfword is unused. */
typedef struct Spec {
    u16 count;                  /* +0x00 */
    u16 pad02;
    u32 value;                  /* +0x04 */
} Spec;

typedef struct Cell {
    u32  count;                 /* +0x00 */
    u32  value;                 /* +0x04 */
    u32 *data;                  /* +0x08 arena slice */
} Cell;

typedef struct Grid {
    u32  status;                /* +0x00 */
    u8   pad04[0x20];
    Cell cells[GRID_WIDTH][GRID_WIDTH];   /* +0x24 */
    s32  width;                 /* +0xC24 */
    s32  height;                /* +0xC28 */
} Grid;

/* 0x0800CB08 */
void FUN_0800cb08(const Spec *spec, s32 x, s32 y, s32 mode)
{
    u16 ime;
    volatile u32 zero;
    Grid *grid;
    Cell *cells;
    Cell *cell;
    const Spec *src;
    u32 *data;
    s32 width;
    s32 i;
    s32 j;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gBuf0201AA80;
    REG_DMA3.control = DMA_FILL_SMALL;
    (void)REG_DMA3.control;
    REG_IME = ime;

    /* The first two stores go through the constant cast and the base local
     * comes AFTER them: that is the only way to get the ROM's `mov r8, r0`
     * order (header comment, point 1). */
    gGrid02015650->width = x >> 10;
    gGrid02015650->height = y >> 10;
    grid = gGrid02015650;

    if (mode == 0)
        gTable0201AA98 = TABLE_MODE_0;
    else if (mode == 1)
        gTable0201AA98 = TABLE_MODE_1;
    else if (mode == 2)
        gTable0201AA98 = TABLE_MODE_2;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    /* The arena pointer comes from the same local as the DMA destination:
     * the ROM keeps using the destination it loaded with `ldr r4` as the
     * allocator inside the loop. */
    data = gArena02016290;
    REG_DMA3.dst = data;
    REG_DMA3.control = DMA_FILL_BIG;
    (void)REG_DMA3.control;
    REG_IME = ime;

    src = spec;
    cells = grid->cells[0];
    width = grid->width;
    for (i = 0; i < width; i++) {
        j = 0;
        /* The guard is read through a `long` view: because that is in a
         * different alias set from the `u32` writes to the cell, agbcc hoists
         * this read out of the outer loop. The plain read below is not
         * hoisted; that is exactly the ROM's pattern (header comment,
         * point 3). */
        if (j < *(long *)&grid->height) {
            cell = cells + i;
            do {
                if (src->count == 0) {
                    cell->count = 0;
                    cell->value = 0;
                    cell->data = 0;
                } else {
                    /* The memory expressions are repeated DELIBERATELY: because
                     * the `bne` target starts a new extended block, agbcc
                     * re-emits the `src->count` read here. Taking it into a
                     * local produces a single read and diverges from the ROM. */
                    cell->count = src->count;
                    cell->value = src->value;
                    cell->data = data;
                }
                data += src->count;
                src++;
                cell += GRID_WIDTH;
                j++;
            } while (j < grid->height);
        }
    }

    grid->status = 0;
    gListHead02016280 = 0;
    gFlag0201AA9C = 0;
}
