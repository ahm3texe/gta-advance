/* Release the two handles at +0x1358 and +0x1360 — 0x08030D84-0x08030DB3
 *
 * Each of the two words is passed to its own routine when it is not null. The
 * base stays in r4 across both calls, which is what the `push {r4}` pays for.
 *
 * 0x1358 comes through the literal pool; 0x1360 is `movs #155 / lsls #5`, the
 * constant-generation idiom docs/GRAM02025810_LAYOUT.md warns is NOT an array
 * stride. Both are scalar offsets.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/release_two_handles.c
 */

#include "gba_types.h"

#define HANDLE_A  0x1358
#define HANDLE_B  (155 << 5)     /* 0x1360 */

extern u8 gRam02025810[];

extern void FUN_0802e048(u32 handle);
extern void FUN_0802ead4(u32 handle);

/* 0x08030D84 */
void FUN_08030d84(void)
{
    u8 *base = gRam02025810;
    u32 handle;

    handle = *(u32 *)(base + HANDLE_A);
    if (handle != 0)
        FUN_0802e048(handle);
    handle = *(u32 *)(base + HANDLE_B);
    if (handle != 0)
        FUN_0802ead4(handle);
}
