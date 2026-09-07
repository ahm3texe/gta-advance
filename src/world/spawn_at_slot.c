/* Yuvaya dogma sarmalayicisi — 0x08028CE8-0x08028D43
 *
 * SelectSlotAB/SelectSlotCD ile secilen yuvadan nesneyi ve tur baytini
 * alip FUN_08024cd8'e sekiz argumanla gecirir; sonucun u8'i doner.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/spawn_at_slot.c
 */

#include "gba_types.h"
typedef struct SlotA { void *obj; } SlotA;
typedef struct SlotB { u8 pad00[28]; u8 *kind; } SlotB;
extern SlotA *SelectSlotAB(u32 which);
extern SlotB *SelectSlotCD(u32 which);
extern u32 FUN_08024cd8(void *obj, u8 a, u32 b, u8 c, u32 d, u32 one, u32 e, u32 kind);
u8 SpawnAtSlot(u8 a, u32 b, u8 c, u32 d, u32 e, u32 which)
{
    SlotA *sa; SlotB *sb;
    sa = SelectSlotAB(which);
    sb = SelectSlotCD(which);
    return FUN_08024cd8(sa->obj, a, b, c, d, 1, e, *sb->kind);
}
