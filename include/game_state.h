#ifndef GUARD_GAME_STATE_H
#define GUARD_GAME_STATE_H

#include "gba_types.h"

/* Game state block: gGameState at 0x02000CE0, 16 bytes.
 *
 * Multiple source files use this symbol. A shared definition avoids conflicting
 * struct layouts, which are rejected by the consistency check. Field layout
 * was established from the ROM:
 *   +0x00 u32  cleared during session initialization at 0x08066144
 *   +0x04 u16  cleared by the same initialization routine
 *   +0x0C u8   two-player mode flag (src/core/nodelist_d5.c,
 *              src/ui/menu_screen.c, src/world/slot_selectors.c)
 */
typedef struct GameState {
    u32 word00;                 /* +0x00 */
    u16 half04;                 /* +0x04: input mask */
    u16 pad06;
    u16 held;                   /* +0x08: keys held in the current frame */
    u16 pressed;                /* +0x0A: held keys with half04 bits excluded */
    u8  flag;                   /* +0x0C */
    u8  pad0D[3];
} GameState;

extern GameState gGameState;

#endif /* GUARD_GAME_STATE_H */
