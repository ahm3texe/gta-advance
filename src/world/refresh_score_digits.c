/* Skor basamaklarini tazeleme — 0x08031294-0x08031317
 *
 * FUN_080625d0'dan sonra gRam02026D90 kuruluysa: gSlotSelector+1 yuvasinin
 * nesnesinin konumundan (>>22) hucre x/y alinir; force sifir ve ikisi de
 * gRam02000008/0A ile ayniysa cikilir; degilse kaydedilip iki ucer
 * basamak (18,10) ve (22,10) konumlarina DrawCounterDigits3 ile yazilir.
 *
 * Bu, docs/BAND_A_ACIK.md'de "sembol yok" diye park edilen uc fonksiyondan
 * biriydi; semboller kaydedilince ilk denemede eslesti.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/refresh_score_digits.c
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
