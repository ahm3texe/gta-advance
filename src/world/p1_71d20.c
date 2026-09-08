/* Sistem cagrisinin hata kodunu cagiran baglama tasi — 0x08071D20.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x08071D20.json. */
#include "gba_types.h"

extern s32 gRam020363C4;
extern s32 FUN_08071b8c(u32);
s32 FUN_08071d20(s32 *error,u32 arg0)
{
    s32 result;
    gRam020363C4 = 0;
    result = FUN_08071b8c(arg0);
    if (result == -1 && gRam020363C4 != 0) *error = gRam020363C4;
    return result;
}
