/* Release an actor's entries — 0x08016808-0x08016869 (+2 padding)
 *
 * Exit if object +0x3C is absent. Release each of four entry bytes unless
 * 0xFF, then set it to 0xFF: +0x8B through ReleaseEntryD; +0xB0/+0xA7/+0x8D
 * through ClearEntry. Release the object with ReleaseObject and clear +0x3C.
 * The same triple occurs in DispatchActorBehavior (dispatch_actor_behavior.c).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_actor_entries.c
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
