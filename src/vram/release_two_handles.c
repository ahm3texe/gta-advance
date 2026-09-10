/* Release the +0x04 and +0x08 handles into one sink — 0x08011D14-0x08011D37
 *
 * Both the sink's address and the block's stay in callee-saved registers across
 * the two calls, which is what the `push {r4, r5}` pays for.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/vram/release_two_handles.c
 */

#include "gba_types.h"

typedef struct HandlePair {
    u32 pad00;
    u32 first;                  /* +0x04 */
    u32 second;                 /* +0x08 */
} HandlePair;

extern u8 gBufferBase02014ED0[];
extern u8 gRam0201AEF0[];

extern void FUN_0800c804(u32 *sink, u32 handle);

/* 0x08011D14 */
void FUN_08011d14(void)
{
    u32 *sink = (u32 *)gBufferBase02014ED0;
    HandlePair *pair = (HandlePair *)gRam0201AEF0;

    FUN_0800c804(sink, pair->first);
    FUN_0800c804(sink, pair->second);
}
