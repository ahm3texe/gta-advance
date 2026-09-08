/* Kart kaynagi tablosundaki ses kaydini isleme ver — 0x080351AC.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x080351AC.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern u8 gCartFlag;
extern Phase1Resource *gRom08CA788C[];
extern void FUN_08032d74(void *data,u32 length,u32 mode,u32 value);
void FUN_080351ac(u32 id)
{
    Phase1Resource *resource;
    u32 index;
    if (gCartFlag == 1 && id > 255) {
        index = id - 201;
        if (index <= 271) {
            resource = gRom08CA788C[index];
            if (resource) FUN_08032d74(resource->data,resource->length,0,255);
        }
    }
}
