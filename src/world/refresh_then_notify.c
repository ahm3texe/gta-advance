/* Test state and notify — 0x08055BBC-0x08055BE7
 *
 * Get the object with FindOrInitAreaNode. If (+0x0B & 0xF1) == 17, call
 * FUN_080536BC; then always call FUN_08055D90.
 *
 * Rule 33: put the mask in a SEPARATE result local and apply &= in place
 * (the ROM constructs the mask FIRST with movs r0,#241).
 * Rule 35: `pop {r0}; bx r0` indicates void.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/refresh_then_notify.c
 */

#include "gba_types.h"
#include "node_list.h"

#define STATE_MASK  0xF1
#define STATE_WANT  17

typedef struct Obj {
    u8 pad00[11];
    u8 state;                   /* +0x0B */
} Obj;


extern Obj *FindOrInitAreaNode(u32 arg);
extern void ClearObjectIdsAndSlots(Obj *obj);
extern void FUN_08055d90(NodeC4 **dest, u32 arg);

/* 0x08055BBC */
void RefreshThenNotify(u32 arg)
{
    Obj *obj;
    u32 state;

    obj = FindOrInitAreaNode(arg);
    state = STATE_MASK;
    state &= obj->state;
    if (state == STATE_WANT)
        ClearObjectIdsAndSlots(obj);
    FUN_08055d90(&gRam02035780, arg);
}
