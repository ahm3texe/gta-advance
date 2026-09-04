/* Gecerli varligin bayragini sorgulama — 0x08050A0C-0x08050A31
 *
 * FUN_08055720'den gelen kimligi 16 bite kirpip 0x7FFF (gecersiz) ile
 * karsilastiriyor; gecerliyse FUN_080561AC sonucunu IsEntityFlagSet'e
 * verip donduruyor, degilse 0.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/check_current_entity.c
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
