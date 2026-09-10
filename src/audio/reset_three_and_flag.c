/* Clear three words, then raise the flag — 0x080348C0-0x080348D7
 *
 * The three offsets share one base register and one zero, and the +0x28 read is
 * what fixes gRam020001B0's recorded size at 44.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/reset_three_and_flag.c
 */

#include "gba_types.h"

typedef struct AudioState {
    u8  pad00[0x10];
    u32 first;                  /* +0x10 */
    u8  pad14[0x10];
    u32 second;                 /* +0x24 */
    u32 third;                  /* +0x28 */
} AudioState;

extern AudioState gRam020001B0;

extern void FUN_08033ba0(void);

/* 0x080348C0 */
void FUN_080348c0(void)
{
    gRam020001B0.first = 0;
    gRam020001B0.second = 0;
    gRam020001B0.third = 0;
    FUN_08033ba0();
}
