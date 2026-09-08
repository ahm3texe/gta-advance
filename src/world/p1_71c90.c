/* Sistem cagrisinin hata kodunu cagiran baglama tasi — 0x08071C90.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x08071C90.json. */
#include "gba_types.h"

extern s32 gRam020363C4;
extern s32 FUN_08071a4c(u32,u32,u32);
s32 FUN_08071c90(s32 *error,u32 arg0,u32 arg1,u32 arg2)
{
    s32 result;
    gRam020363C4 = 0;
    result = FUN_08071a4c(arg0,arg1,arg2);
    if (result == -1 && gRam020363C4 != 0) *error = gRam020363C4;
    return result;
}
