/* Attach an audio source to the object and set its level — 0x08035230.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08035230.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1Resource *gRom08CA788C[];
extern u32 FUN_08033ad8(u32);
extern u32 FUN_08032d74(void *,u32,u32,u32);
extern void FUN_08032fc4(u32,u32);
void FUN_08035230(Phase1SoundActor *actor,u32 id)
{
    u32 index;
    Phase1Resource *resource;
    u32 handle;
    if (actor && actor->sound && !FUN_08033ad8(0)) {
        index = id - 201;
        if (index <= 271) {
            resource = gRom08CA788C[index];
            if (resource) {
                handle = FUN_08032d74(resource->data,resource->length,0,14);
                actor->sound->handle = handle;
                FUN_08032fc4(handle,32768);
            }
        }
    }
}
