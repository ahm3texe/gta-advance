/* Submit the audio record from the cartridge resource table — 0x08035168.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08035168.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u8 gCartFlag;
extern Phase1Resource *gRom08CA788C[];
extern void FUN_08032d74(void *data,u32 length,u32 mode,u32 value);
void FUN_08035168(u32 id)
{
    Phase1Resource *resource;
    u32 index;
    if (gCartFlag == 1 && id > 255) {
        index = id - 201;
        if (index <= 271) {
            resource = gRom08CA788C[index];
            if (resource) FUN_08032d74(resource->data,resource->length,2,6);
        }
    }
}
