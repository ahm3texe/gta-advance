/* Query the current entity's flag — 0x08050A0C-0x08050A31
 *
 * Truncate the ID from FUN_08055720 to 16 bits and compare against 0x7FFF
 * (invalid). If valid, pass the result of FUN_080561AC to IsEntityFlagSet and
 * return its result; otherwise return 0.
 *
 * Rule 35: `pop {r1}; bx r1` means r0 carries a return value; the return type is u32.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/check_current_entity.c
 */

#include "gba_types.h"

#define ID_INVALID 0x7FFF

extern u32 FUN_08055720(void);
extern u32 FUN_080561ac(u32 id);
extern u32 IsEntityFlagSet(u32 arg);

/* 0x08050A0C */
u32 CheckCurrentEntity(void)
{
    u16 id;

    id = FUN_08055720();
    if (id == ID_INVALID)
        return 0;
    return IsEntityFlagSet(FUN_080561ac(id));
}
