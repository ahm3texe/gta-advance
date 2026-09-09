/* Copy and mirror a packet — 0x0801D848-0x0801D869
 *
 * Copy the caller's 12 bytes to the start of the object, mirror its first
 * 116 bytes to +0x13C, then call FUN_0801A560.
 *
 * Rule 32: structure assignment emits an ldmia/stmia pair. The ROM constructs
 * 0x13C with movs r1,#158 / lsls r1,#1 because 316 does not fit the immediate;
 * a plain constant would produce a pool load.
 * Rule 35: `pop {r0}; bx r0` indicates void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/init_and_mirror.c
 */

#include "gba_types.h"

#define MIRROR_OFFSET (158 << 1)
#define MIRROR_SIZE   116

typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

extern void FUN_0806dbd0(u8 *dest, u8 *src, u32 size);
extern void FUN_0801a560(void *obj);

/* 0x0801D848 */
void InitAndMirror(u8 *obj, Pack12 *src)
{
    *(Pack12 *)obj = *src;
    FUN_0806dbd0(obj + MIRROR_OFFSET, obj, MIRROR_SIZE);
    FUN_0801a560(obj);
}
