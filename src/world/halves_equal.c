/* Equality of two halfwords — 0x08030BE8-0x08030C0B  (36 bytes, BYTE-MATCHING)
 *
 * Filters the +0x04 and +0x1C halfwords of gRam02025810 through 0xFFFF and
 * compares them.  The ROM loads the base pointer and the mask from the pool,
 * copies the mask with `adds r2,r1,#0` and uses the copy on the left-hand
 * side; on the right-hand side the mask is dead, so it does `ands r1,r0` in
 * place.
 *
 * The ROM (measured):
 *   ldr r0,[pc]   -> 0x02025810      ldr r1,[pc] -> 0x0000ffff
 *   adds r2,r1,#0                    <- the mask's copy (the left-hand temp)
 *   ldrh r3,[r0,#4]  / ands r2,r3    <- the left-hand side comes ENTIRELY first
 *   ldrh r0,[r0,#28] / ands r1,r0    <- the base pointer dies here
 *   cmp r2,r1 / beq -> the `return 1` body, moved to the end (rule 49)
 *   falling through: movs r0,#0 ; b epilogue ; pool ; movs r0,#1 ; bx lr
 *
 * WHAT MAKES IT MATCH -- THE MASK MUST BE IN A SEPARATE LOCAL.
 * Because the fields are u16, writing `& 0xFFFF` as a constant lets agbcc
 * treat it as redundant and delete it, and the output drops to 24 bytes (35
 * off).  Taken into a local variable, the ANDs survive and the size settles at
 * 36.
 *
 * THE SECOND AND REAL DETAIL -- THE ORDER OF DEFINITION (newly measured):
 * The order in which the pointer and the mask are FIRST DEFINED decides both
 * the pool order and the register allocation.  The pointer must be defined
 * first.
 *   b = ...; mask = 0xFFFF;   -> r0=base, r1=mask, pool [0x02025810,
 *                                0x0000ffff]  => 0 off
 *   mask = 0xFFFF; b = ...;   -> r0=mask, r2=base, the pool REVERSED
 *                                (0x0000ffff first) => 15 off
 * So the earlier round's "36 bytes / 15 off" was not a wrong spelling, only a
 * reversed definition order.  Swapping the two lines took 15 off to 0.
 *
 * SPELLINGS TRIED AND REJECTED:
 *  - `b->first & 0xFFFF` (the constant directly): 24 bytes, 35 off.  The mask
 *    is dropped.
 *  - Making the fields u32: 24 bytes; the mask was dropped again, and ldr came
 *    out instead of ldrh.
 *  - Two separate mask locals (m1, m2): 36 bytes / 17 off.  The ROM produces a
 *    copy from a single mask local, not from two separate ones.
 *  - Every spelling with `mask` defined first (both declaration order and
 *    assignment order): 36 bytes / 15 off.  Do not try it again.
 * NOTE: whether the mask's type is u32 or u16 makes no difference -- both give
 * 0 off (measured).  u32 was kept.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/halves_equal.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define HALF_MASK 0xFFFF

typedef struct Block {
    u8  pad00[4];
    u16 first;                  /* +0x04 */
    u8  pad06[22];
    u16 second;                 /* +0x1C */
} Block;

/* 0x08030BE8 */
u32 HalvesEqual(void)
{
    Block *b;
    u32 mask;

    /* gRam02025810 is shared raw storage (include/ram_symbols.h);
       each translation unit casts to its own view LOCALLY. */
    b = (Block *)gRam02025810;
    mask = HALF_MASK;           /* THE ORDER MATTERS: AFTER the pointer */
    if ((b->first & mask) == (b->second & mask))
        return 1;
    return 0;
}
