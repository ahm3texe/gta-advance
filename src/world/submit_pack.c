/* Submit a 12-byte packet — 0x0803C7D4-0x0803C7EB
 *
 * Copy the caller's 12 bytes to gRam0202F360 and call FUN_08060db4.
 * Rule 32: structure assignment emits the ldmia/stmia pair.
 * Rule 35: the final pop {r0}; bx r0 indicates void; a u32 return would
 * keep r0 live.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/submit_pack.c
 */

#include "gba_types.h"

typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

extern Pack12 gRam0202F360;

extern void FUN_08060db4(Pack12 *pack, u32 arg);

/* 0x0803C7D4 */
void SubmitPack(Pack12 *src, u32 arg)
{
    gRam0202F360 = *src;
    FUN_08060db4(src, arg);
}
