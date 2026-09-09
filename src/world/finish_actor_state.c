/* Finish an actor state — 0x0801979C-0x080197FB
 *
 * If state +0x28 is not 0x7FFF (empty): when +0xA5 (s8) is nonzero, set
 * +0x0A = 2 and call RequestActorAction(state,15,2); otherwise use (state,4,2).
 * If visual +0x3C exists, set its mode +0x26 to 33. Then set state to 0x7FFF
 * and call FUN_08016990(0x10000). Layout matches actor_state_step.c
 * (STATE_IDLE, sub, visual).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/finish_actor_state.c
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
