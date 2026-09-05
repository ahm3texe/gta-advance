/* Durum sinayip bildirme — 0x08055BBC-0x08055BE7
 *
 * FUN_08054744 ile nesneyi alip +0x0B baytinin 0xF1 maskesi 17 ise
 * FUN_080536BC'yi cagiriyor, ardindan her durumda FUN_08055D90'i
 * cagiriyor.
 *
 * Kural 33: maske AYRI sonuc yereline konup yerinde `&=` yapiliyor
 * (ROM `movs r0,#241` ile maskeyi ONCE kuruyor).
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/refresh_then_notify.c
 */

#include "gba_types.h"
#include "node_list.h"

#define STATE_MASK  0xF1
#define STATE_WANT  17

typedef struct Obj {
    u8 pad00[11];
    u8 state;                   /* +0x0B */
} Obj;


extern Obj *FUN_08054744(u32 arg);
extern void ClearObjectIdsAndSlots(Obj *obj);
extern void FUN_08055d90(NodeC4 **dest, u32 arg);

/* 0x08055BBC */
void RefreshThenNotify(u32 arg)
{
    Obj *obj;
    u32 state;

    obj = FUN_08054744(arg);
    state = STATE_MASK;
    state &= obj->state;
    if (state == STATE_WANT)
        ClearObjectIdsAndSlots(obj);
    FUN_08055d90(&gRam02035780, arg);
}
