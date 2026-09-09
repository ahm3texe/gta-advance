/* Frame call chain — 0x08028E6C-0x08028E85
 *
 * Five-function chain: FUN_08029130, FUN_08027f48, FUN_08019800,
 * FUN_08019a64, ProcessFirstEntry.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/frame_chain.c
 */

#include "gba_types.h"

extern void FUN_08029130(void);
extern void FUN_08027f48(void);
extern void FUN_08019800(void);
extern void FUN_08019a64(void);
extern void ProcessFirstEntry(void);

/* 0x08028E6C */
void FrameChain(void)
{
    FUN_08029130();
    FUN_08027f48();
    FUN_08019800();
    FUN_08019a64();
    ProcessFirstEntry();
}
