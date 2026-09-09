/* Twin wrappers — 0x08063E38-0x08063E57
 *
 * Both truncate parameter two to u16 and pass it as the FIRST argument to
 * FUN_080638a0. Only the second argument differs: 0 at 0x08063E38, 1 at
 * 0x08063E48. Parameter one (r0) is UNUSED; the ROM never reads it.
 *
 * RETURN: the ROM uses pop {r1}; bx r1, not pop {r0}; bx r0, preserving r0.
 * These wrappers forward the callee's result and are not void (the converse
 * of rule 35: return through another register leaves the value in r0).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/queue_glyph_wrappers.c
 */

#include "gba_types.h"

extern u32 FUN_080638a0(u16 value, u32 flag);

/* 0x08063E38 */
u32 ForwardU16WithFlag0(s32 unused, u16 value)
{
    return FUN_080638a0(value, 0);
}

/* 0x08063E48 */
u32 ForwardU16WithFlag1(s32 unused, u16 value)
{
    return FUN_080638a0(value, 1);
}
