/* Read the field at +0x100 — 0x0801D8B0-0x0801D8BB
 *
 * The ROM constructs the offset with 0x80 << 1 (movs #128 + lsls #1).
 * 256 does not fit the immediate, so the shifted form is required; writing
 * 256 directly would produce a literal-pool load.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_field_100.c
 */

#include "gba_types.h"

/* 0x0801D8B0 */
u32 GetField100(u8 *base)
{
    return *(u32 *)(base + (0x80 << 1));
}
