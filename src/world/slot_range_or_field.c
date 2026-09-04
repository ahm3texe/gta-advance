/* Kip 2 ise yuva araligi, degilse alan — 0x08038044-0x0803805D
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_range_or_field.c
 */

#include "gba_types.h"

#define KIND_SLOT 2

typedef struct Sub {
    u8  pad00[8];
    u32 value;                  /* +0x08 */
} Sub;

typedef struct Obj {
    u8   pad00[8];
    u8   kind;                  /* +0x08 */
    u8   pad09[39];
    Sub *sub;                   /* +0x30 */
} Obj;

extern u32 FUN_0803c400(Obj *obj);
extern u32 GetSlotRange(u32 arg);

/* 0x08038044 */
u32 SlotRangeOrField(Obj *obj)
{
    if (obj->kind == KIND_SLOT)
        return GetSlotRange(FUN_0803c400(obj));
    return obj->sub->value;
}
