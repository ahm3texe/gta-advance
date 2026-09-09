/* Mark distant actors, skipping the current phase — 0x0806262C-0x08062653
 *
 * The low two bits of gRam02000224 name a phase. Each of the two passes runs
 * unless the phase is its own, so the two of them together cover every value
 * except that one.
 *
 * The word's ADDRESS and the mask 3 both stay in callee-saved registers across
 * the calls, which is what the `push {r4, r5}` pays for. The word itself is
 * re-read after the first call; the ROM does not keep it.
 *
 * Reading it into a local before the mask is declared is what settles which of
 * the two gets r4 and which r5. With `gRam02000224 & mask` written directly in
 * both tests, agbcc emits the mask's `movs` before the address's pool load and
 * the two registers come out swapped; three spellings were measured that way
 * before the explicit read fixed the order.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/mark_distant_by_phase.c
 */

#include "gba_types.h"

#define PHASE_MASK  3

extern u32 gRam02000224;

extern void MarkDistantActors(void);
extern void MarkDistantActorsB(void);

/* 0x0806262C */
void FUN_0806262c(void)
{
    u32 value = gRam02000224;
    u32 mask = PHASE_MASK;

    if ((value & mask) != 1)
        MarkDistantActors();
    value = gRam02000224;
    if ((value & mask) != 2)
        MarkDistantActorsB();
}
