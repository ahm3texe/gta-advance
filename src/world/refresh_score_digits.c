/* Refresh score digits — 0x08031294-0x08031317
 *
 * After FUN_080625d0, if gRam02026D90 is set, obtain cell x/y from the object
 * position in slot gSlotSelector+1 (>>22). Exit if force is zero and both
 * match gRam02000008/0A. Otherwise save them and use DrawCounterDigits3 to
 * write two three-digit values at (18,10) and (22,10).
 *
 * One of three functions parked for missing symbols in BAND_A_OPEN_ISSUES.md;
 * it matched on the first attempt once the symbols were registered.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/refresh_score_digits.c
 */

#include "gba_types.h"
typedef struct Pos { s32 x; s32 y; } Pos;
typedef struct Obj { u8 pad00[24]; Pos *pos; } Obj;
typedef struct SlotA { Obj *obj; } SlotA;
extern u32 gRam02026D90;
extern s16 gSlotSelector;
extern u16 gRam02000008;
extern u16 gRam0200000A;
extern void   FUN_080625d0(void);
extern SlotA *SelectSlotAB(u32 which);
extern void   DrawCounterDigits3(s32 value, s32 x, s32 y);
void RefreshScoreDigits(u32 force)
{
    u16 cx; u16 cy;
    FUN_080625d0();
    if (gRam02026D90 == 0)
        return;
    cx = SelectSlotAB(gSlotSelector + 1)->obj->pos->x >> 22;
    cy = SelectSlotAB(gSlotSelector + 1)->obj->pos->y >> 22;
    if (force == 0 && cx == gRam02000008 && cy == gRam0200000A)
        return;
    gRam02000008 = cx;
    gRam0200000A = cy;
    DrawCounterDigits3(cx, 18, 10);
    DrawCounterDigits3(cy, 22, 10);
}
