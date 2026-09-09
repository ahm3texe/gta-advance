/* Area cleanup -- 0x08030CB4-0x08030D0B, 88 bytes.  MATCHES (0 off).
 *
 * If the pending-cleanup flag is set it fills three half words into each of
 * two VRAM rows, releases a block and clears the flag.
 *
 * THE SOLUTION: the loop form is ARRAY INDEXING, NOT POINTER INCREMENTS.
 * Every spelling that loaded the pointers (left/right) into locals before the
 * loop got stuck at 7 bytes; the indexed spelling (TILE_ROW_LEFT[i] = ...)
 * gave 0.  The generated code is the same: the two pointers still advance by 2
 * -- but now it is agbcc's strength reduction that produces them, not the
 * SOURCE.  The only visible difference is INSTRUCTION ORDER, and that order
 * was the whole 7 bytes.
 *
 * THE MECHANISM (measured; it is the only thing that explains the difference):
 * The ROM's pre-loop order is this:
 *     movs r3, #0          i = 0            <- a plain statement
 *     ldr  r4, =0x02026E80 block            <- a plain statement
 *     ldr  r1, =0xF0E8     the fill constant <- a HOISTED loop invariant
 *     adds r0, r1, #0      a copy           <- cse2 turning the constant load
 *                                              into a copy
 *     ldr  r2, =0x06009858 right            <- a STRENGTH REDUCTION start
 *     ldr  r1, =0x06009818 left             <- a STRENGTH REDUCTION start
 * The critical point: in loop optimisation agbcc first writes the invariants
 * (movables) before loop_start, and then writes the pointer starts produced by
 * strength reduction, also before loop_start.  The second insertion comes
 * AFTER the first.  So the hoisted constant + copy come BEFORE the pointer
 * loads.  If you write the pointers as plain statements in the source, they
 * come FIRST in the preheader and the hoisted constant LAST -- exactly the
 * reverse of the ROM.  That order difference never closed.
 *
 * TWO SIDE MEASUREMENTS confirming the same mechanism:
 *   - The `ldr r1, =constant` + `adds r0, r1, #0` pair does NOT come from a C
 *     copy.  Written inline INSIDE the loop, agbcc hoists the constant out;
 *     cse2 turns the hoisted load into a copy (the value is already in a
 *     register) and the copy can no longer be deleted.  Writing
 *     `fill2 = fill;` at source level DOES NOT PRODUCE THIS -- cse1 propagates
 *     the constant, the copy dies, and the output is 84 bytes (the ROM has
 *     88).  Keeping both fills live to preserve the copy, on the other hand,
 *     needs an extra callee-saved register: push {r4,r5,lr}.
 *   - Both spellings (a fill variable / an inline constant) use the same
 *     constant but its place in the pool differs; the pool order follows the
 *     order of the ldr instructions.
 *
 * SPELLINGS RULED OUT (all measured, do not try them again):
 *   The pointer-increment loop family -- none of them got below 7:
 *     both fill and fill2 live (the previous best)          7
 *     swapping the use of fill/fill2                        7
 *     a two-copy chain (fill2=fill; fill=fill2)             7
 *     moving the block assignment before i                 10
 *     an inline constant + a fill variable mixed (both ways) 12
 *     an inline constant inside the loop, no fill          21
 *     loading fill before the loop and inline inside       21
 *     fill2 in both stores (the copy dies, 84 bytes)       61
 *     moving the copy inside the loop                      61
 *     a u32->u16 / u16->u32 / s16 copy                     61
 *     a three-copy chain, the `register` keyword           61
 *     int fill, a for loop + inline                        61
 *     *right = *left = fill  /  *left = *right = fill   61/60
 *     moving the copy after the pointer assignments        64
 *     assigning fill inside the loop                       64
 *   The indexed loop family -- only an ordering detail remained:
 *     writing RIGHT first                                   2
 *     the for (i = 0; i <= 2; i++) form                     4
 *     using a fill variable (the constant is not hoisted)  61
 *   From earlier rounds (the structure is obsolete, but keep the note):
 *     a rule 40 narrow volatile read of fill2              85
 *     not loading the block before the loop                23
 *     making the counter s32                               22
 *   The declaration order has NO EFFECT in this function (all three
 *   permutations are the same).
 *   The permuter ran 8,871 iterations and did not get below 7: its score is an
 *   instruction-weighted heuristic, not byte equality; do not trust its
 *   intermediate scores.
 *
 * THE GENERAL LESSON (a complement to rule 49): if a loop in the ROM
 * increments a pointer, that DOES NOT mean a pointer is incremented IN THE
 * SOURCE.  Look at the pre-loop instruction order: if the constant loads come
 * BEFORE the pointer loads, the pointers come from strength reduction, i.e.
 * the source is written with indexing.
 *
 * The three matching functions in the same set: src/world/area_flags.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/area_cleanup.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define TILE_FILL      0xF0E8
#define TILE_RUN       3
#define TILE_ROW_LEFT   ((u16 *)0x06009818)
#define TILE_ROW_RIGHT  ((u16 *)0x06009858)

typedef struct Progress {
    u8 pad0000[0x137E];
    u8 pendingCleanup;          /* +0x137E */
} Progress;

extern u32      gRam02026E80;

extern void ReleaseObject(u32 *block);

/* 0x08030CB4 */
void CleanupAreaTiles(void)
{
    u32 *block;
    u32 i;

    if (((Progress *)gRam02025810)->pendingCleanup == 0)
        return;

    i = 0;
    block = &gRam02026E80;
    do {
        TILE_ROW_LEFT[i] = TILE_FILL;
        TILE_ROW_RIGHT[i] = TILE_FILL;
        i++;
    } while (i <= TILE_RUN - 1);

    ReleaseObject(block);
    ((Progress *)gRam02025810)->pendingCleanup = 0;
}
