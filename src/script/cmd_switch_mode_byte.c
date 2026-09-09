/* Script command: change the mode byte — 0x0805B920-0x0805B949
 *
 * Writes the operand into gRam02027EE0 only when it differs from what is
 * already there, and then calls one of two routines depending on whether the
 * new value is zero. An operand equal to the current value does nothing at all.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_switch_mode_byte.c
 */

#include "gba_types.h"

extern u8 gRam02027EE0;

extern void FUN_0803493c(void);
extern void FUN_080348c0(void);

/* 0x0805B920 */
u32 FUN_0805b920(u32 a, u16 mode)
{
    if (gRam02027EE0 != mode) {
        gRam02027EE0 = mode;
        if (mode == 0)
            FUN_0803493c();
        else
            FUN_080348c0();
    }
    return 1;
}
