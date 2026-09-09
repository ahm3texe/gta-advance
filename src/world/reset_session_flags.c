/* Reset session flags — 0x08066540-0x08066567
 *
 * Clear four locations: gSlotSelector (halfword), gGameState +12 (byte),
 * gVBlankEnabled (halfword), and gRam02036328 (byte).
 *
 * The ROM uses TWO zero registers for the halfword stores (r1/r2),
 * initializing the second with movs r2,#0 before the third write.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/reset_session_flags.c
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
