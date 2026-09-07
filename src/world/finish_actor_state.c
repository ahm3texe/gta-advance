/* Aktor durumunu bitirme — 0x0801979C-0x080197FB
 *
 * +0x28 durumu 0x7FFF (bos) degilse: +0xA5 (s8) sifir degilse +0x0A = 2
 * ve RequestActorAction(durum,15,2), degilse (durum,4,2); +0x3C gorunumu
 * varsa +0x26 kipi 33. Sonra durum 0x7FFF'e alinip FUN_08016990(0x10000).
 * Yerlesim src/world/actor_state_step.c ile ayni (STATE_IDLE, sub, visual).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/finish_actor_state.c
 */

#include "gba_types.h"
#define STATE_IDLE  0x7FFF
#define VISUAL_DONE 33
typedef struct Visual { u8 pad0[0x26]; u8 mode; } Visual;
typedef struct Actor {
    u8 pad00[0x0A]; u8 sub; u8 pad0b[0x1D]; u32 state; u8 pad2c[0x10]; Visual *visual;
    u8 pad40[0x65]; s8 flagA5;
} Actor;
extern void RequestActorAction(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_08016990(Actor *self, s32 a);
void FinishActorState(Actor *self)
{
    u32 state;
    state = self->state;
    if (state != STATE_IDLE) {
        if (self->flagA5 != 0) {
            self->sub = 2;
            RequestActorAction(self, state, 15, 2);
        } else {
            RequestActorAction(self, state, 4, 2);
        }
        if (self->visual != 0)
            self->visual->mode = VISUAL_DONE;
    }
    self->state = STATE_IDLE;
    FUN_08016990(self, 0x10000);
}
