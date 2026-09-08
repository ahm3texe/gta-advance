/* Hedefli eylem sonucunu kaydet ve bitis bayragini kur — 0x0803F910.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0803F910.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u32 FUN_0804fc48(void *target,void *owner);
extern u16 FUN_08023654(void *owner,u32 value);
u32 FUN_0803f910(Phase1Action *action)
{
    u32 result;
    void *owner = action->owner;
    action->result = FUN_08023654(owner,FUN_0804fc48(action->target,owner));
    if (action->result == 1) result = 0;
    else { action->done = 1; result = 1; }
    return result;
}
