/* Audio event on a twelve-tick interval — 0x0805ABDC.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0805ABDC.json. */
#include "gba_types.h"

extern u32 gRam02000224,gRam02035B40;
extern void FUN_08035058(void *,u32);
void FUN_0805abdc(void *actor)
{
    if (gRam02000224 < gRam02035B40 || gRam02000224 >= gRam02035B40 + 12) {
        FUN_08035058(actor,263);
        gRam02035B40 = gRam02000224;
    }
}
