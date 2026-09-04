/* Oturum bayraklarini sifirlama — 0x08066540-0x08066567
 *
 * Dort ayri yeri sifirliyor: gSlotSelector (yarim soz), gGameState +12
 * (bayt), gVBlankEnabled (yarim soz) ve gRam02036328 (bayt).
 *
 * ROM yarim soz yazarken IKI ayri sifir register'i kullaniyor (r1 ve r2);
 * ucuncu yazmadan once `movs r2,#0` ile ikincisini kuruyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/reset_session_flags.c
 */

#include "gba_types.h"

typedef struct GameState {
    u8 pad00[12];
    u8 flag;                    /* +0x0C */
} GameState;

extern u16       gSlotSelector;
extern GameState gGameState;
extern u16       gVBlankEnabled;
extern u8        gRam02036328;

/* 0x08066540 */
void ResetSessionFlags(void)
{
    u16 zero;

    gSlotSelector = 0;
    gGameState.flag = 0;
    zero = 0;
    gVBlankEnabled = zero;
    gRam02036328 = 0;
}
