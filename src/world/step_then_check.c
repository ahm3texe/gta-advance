/* Step and test a flag — 0x08031DB8-0x08031DDB
 *
 * Call FUN_0802EF3C, then call FUN_08029C20 if the word at
 * gRam02025810+0x1358 is nonzero. The ROM loads base and offset SEPARATELY
 * and adds them; 0x1358 does not fit an eight-bit immediate.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/step_then_check.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

typedef struct Progress {
    u8  pad0000[0x1358];
    u32 pending;                /* +0x1358 */
} Progress;

extern void FUN_0802ef3c(void);
extern void FUN_08029c20(void);

/* 0x08031DB8 */
void StepThenCheck(void)
{
    FUN_0802ef3c();
    if (((Progress *)gRam02025810)->pending != 0)
        FUN_08029c20();
}
