/* Settle only when nothing is held — 0x0805A974-0x0805A9A5
 *
 * Answers 0 without doing anything while gFrameCounterEwram is still zero, and
 * also when the +0x1358 handle is set. Only the first of the two handles is
 * consulted here, unlike the siblings at 0x0805A824 and 0x0805A928.
 *
 * Rule 49: the zero answer is at the end and both tests branch forward to it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_settle_if_free.c
 */

#include "gba_types.h"

#define HANDLE_A  0x1358

extern u32 gFrameCounterEwram;
extern u8  gRam02025810[];

extern void FUN_080308ec(void);

/* 0x0805A974 */
u32 FUN_0805a974(void)
{
    u8 *base;

    if (gFrameCounterEwram == 0) goto no;
    base = gRam02025810;
    if (*(u32 *)(base + HANDLE_A) != 0) goto no;
    FUN_080308ec();
    return 1;
no:
    return 0;
}
