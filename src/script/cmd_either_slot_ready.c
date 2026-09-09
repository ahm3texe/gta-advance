/* Script command: is slot 1 or slot 2 ready — 0x0805A1A8-0x0805A1D5
 *
 * Slot 1 alone is enough. Slot 2 only counts when byte 12 of gGameState is
 * non-zero, which is the byte data/ram_map.csv records as selecting the VBlank
 * frame-delay behaviour.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_either_slot_ready.c
 */

#include "gba_types.h"

#define TWO_PLAYER  12          /* gGameState byte 12 */

extern u8 gGameState[];

extern u32 FUN_0803c3d4(u32 slot);

/* 0x0805A1A8 */
u32 FUN_0805a1a8(void)
{
    if (FUN_0803c3d4(1) != 0) goto yes;
    if (gGameState[TWO_PLAYER] == 0) goto no;
    if (FUN_0803c3d4(2) == 0) goto no;
yes:
    return 1;
no:
    return 0;
}
