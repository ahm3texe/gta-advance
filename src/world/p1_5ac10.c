/* On iki tik aralikli ses olayi — 0x0805AC10.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0805AC10.json. */
#include "gba_types.h"

extern u32 gRam02000224,gRam020302E4;
extern void FUN_08035058(void *,u32);
void FUN_0805ac10(void *actor)
{
    if (gRam02000224 < gRam020302E4 || gRam02000224 >= gRam020302E4 + 12) {
        FUN_08035058(actor,267);
        gRam020302E4 = gRam02000224;
    }
}
