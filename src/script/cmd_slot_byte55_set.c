/* Script command: is byte 55 of the current slot set — 0x0805B9BC-0x0805B9E1
 *
 * The slot is chosen by gRam02000F00. Byte 55 is the same one
 * src/ui/init_menu_screen.c tests before it draws.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_slot_byte55_set.c
 */

#include "gba_types.h"

#define SLOT_READY  55

extern u8 gRam02000F00;

extern u8 *SelectSlotAB(u32 which);

/* 0x0805B9BC */
u32 FUN_0805b9bc(void)
{
    u8 *slot = SelectSlotAB(gRam02000F00);

    if (slot != 0) {
        if (slot[SLOT_READY] != 0) goto yes;
    }
    return 0;
yes:
    return 1;
}
