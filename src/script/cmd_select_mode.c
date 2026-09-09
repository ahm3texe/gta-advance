/* Script commands: select a mode — 0x0805B770-0x0805B79D
 *
 * Three adjacent handlers taking no operand, each passing its own constant to
 * FUN_0803AB0C. The constants are 3, 1 and 2 in ROM order, so they are not an
 * enumeration laid out in sequence; whatever they select, the table's order does
 * not follow it.
 *
 * The three sit next to each other in the ROM with nothing between them, so
 * they share a file. FUN_0805B7A0 onwards is a different shape and does not.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_select_mode.c
 */

#include "gba_types.h"

extern void FUN_0803ab0c(u32 mode);

/* 0x0805B770 */
u32 FUN_0805b770(void)
{
    FUN_0803ab0c(3);
    return 1;
}

/* 0x0805B780 */
u32 FUN_0805b780(void)
{
    FUN_0803ab0c(1);
    return 1;
}

/* 0x0805B790 */
u32 FUN_0805b790(void)
{
    FUN_0803ab0c(2);
    return 1;
}
