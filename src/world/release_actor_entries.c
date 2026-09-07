/* Aktorun girislerini birakma — 0x08016808-0x08016869 (+2 dolgu)
 *
 * +0x3C nesnesi yoksa cikar. Dort giris bayti (+0x8B ReleaseEntryD ile,
 * +0xB0/+0xA7/+0x8D ClearEntry ile) 0xFF degilse birakilip 0xFF yapilir;
 * sonra nesne ReleaseObject ile birakilip +0x3C sifirlanir. Ayni uclu
 * DispatchActorBehavior'da da var (dispatch_actor_behavior.c).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_actor_entries.c
 */

#include "gba_types.h"
#define ENTRY_NONE 0xFF
typedef struct Actor { u8 pad00[0x3C]; u8 *obj; u8 pad40[0x4B]; u8 entryD; u8 pad8c; u8 entryC; u8 pad8e[0x19]; u8 entryB; u8 pada8[8]; u8 entryA; } Actor;
extern u32  ReleaseEntryD(u8 idx);
extern void ClearEntry(u8 index);
extern void ReleaseObject(u8 *obj);
void ReleaseActorEntries(Actor *self)
{
    if (self->obj == 0)
        return;
    if (self->entryD != ENTRY_NONE) {
        ReleaseEntryD(self->entryD);
        self->entryD = ENTRY_NONE;
    }
    if (self->entryA != ENTRY_NONE) {
        ClearEntry(self->entryA);
        self->entryA = ENTRY_NONE;
    }
    if (self->entryB != ENTRY_NONE) {
        ClearEntry(self->entryB);
        self->entryB = ENTRY_NONE;
    }
    if (self->entryC != ENTRY_NONE) {
        ClearEntry(self->entryC);
        self->entryC = ENTRY_NONE;
    }
    ReleaseObject(self->obj);
    self->obj = 0;
}
