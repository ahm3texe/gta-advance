#ifndef GUARD_GAME_STATE_H
#define GUARD_GAME_STATE_H

#include "gba_types.h"

/* Oyun durumu blogu — gGameState @ 0x02000CE0, 16 bayt.
 *
 * Birden fazla kaynak bu sembolu kullaniyor; ayni sembol icin farkli
 * struct govdeleri tanimlamak tutarlilik denetimini durduruyor, bu
 * yuzden tek tanim burada. Alan yerlesimi ROM'dan olculdu:
 *   +0x00 u32  0x08066144 oturum kurulumunda sifirlaniyor
 *   +0x04 u16  ayni yerde sifirlaniyor
 *   +0x0C u8   iki oyunculu kip bayragi (src/core/nodelist_d5.c,
 *              src/ui/menu_screen.c, src/world/slot_selectors.c)
 */
typedef struct GameState {
    u32 word00;                 /* +0x00 */
    u16 half04;                 /* +0x04 */
    u8  pad06[6];
    u8  flag;                   /* +0x0C */
    u8  pad0D[3];
} GameState;

extern GameState gGameState;

#endif /* GUARD_GAME_STATE_H */
