/* Pool +0 getter — 0x08037F70-0x08037F7B
 *
 * Return the FIRST field of gRam0202F300. Four siblings reading +8 from pool
 * symbols are in src/world/pool_gets.c.
 *
 * SEPARATE FILE: adding this to pool_gets.c moved the translation unit's
 * shared literal pool and broke matching MarkAndClear's ldr [pc,#imm]
 * (34/34 -> 1/34). Keep noncontiguous address clusters in separate files.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/pool_first.c
 */

#include "gba_types.h"

typedef struct Pool {
    u32 first;                  /* +0x00 */
    u8  pad04[4];
    u32 value;                  /* +0x08 */
} Pool;

extern Pool gRam0202F300;

/* 0x08037F70 */
u32 GetPoolBFirst(void) { return gRam0202F300.first; }
