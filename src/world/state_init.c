/* Durum kurulumu ve degistirici — 0x0803F630-0x0803F67B
 *
 * Bir 132+ baytlik struct'in ilklendirilmesi (0x0803F630), bos `bx lr`
 * (0x0803F64C), alt nesne cagricisi (0x0803F650) ve 3 sozcuk kopyalayip
 * bayrak/durum sifirlayan degistirici (0x0803F65C).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/state_init.c
 */

#include "gba_types.h"

typedef struct StateSlot {
    u8 pad00[20];
    u32 unk14;                  /* +0x14 */
    u32 unk18;                  /* +0x18 */
} StateSlot;

typedef struct State {
    void       *ptr0;           /* +0x00 */
    StateSlot  *slot;           /* +0x04 */
    u32         unk08;          /* +0x08 */
    u8          pad0C[0x20];
    u32         unk2C;          /* +0x2C */
    u8          pad30[0x51];
    u8          unk81;          /* +0x81 */
    u8          unk82;          /* +0x82 */
    u8          unk83;          /* +0x83 */
} State;

typedef struct Params {
    u32 a;
    u32 b;
    u32 c;
} Params;

extern void FUN_08041ef0(void *arg);

/* 0x0803F630 */
void InitState(State *s, void *ptr, StateSlot *slot)
{
    s->ptr0 = ptr;
    s->slot = slot;
    s->unk08 = 0;
    s->unk82 = 0;
    s->unk83 = 0;
    s->unk2C = 0;
    s->unk81 = 0;
}

/* 0x0803F64C */
void StateNoop(void)
{
}

/* 0x0803F650 */
void RunState(State *s)
{
    FUN_08041ef0(s->ptr0);
}

/* 0x0803F65C */
void UpdateState(State *s, const Params *params)
{
    *(Params *)s->slot = *params;
    s->slot->unk18 = 0;
    s->slot->unk14 = 0;
    s->unk81 = 1;
    s->unk08 = 0;
}
