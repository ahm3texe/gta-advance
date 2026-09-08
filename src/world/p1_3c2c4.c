/* Etkin yuvanin sonraki veya onceki girdisini sec — 0x0803C2C4.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0803C2C4.json. */
#include "gba_types.h"
#include "phase1_types.h"
#include "ram_symbols.h"

extern Ctx *gRam02000F08;   /* band_0803afbc.c ile ayni gorunum */
extern u8 gRam02000F00;
extern void FUN_080384f0(void);
extern u32 FUN_0803c320(s32,u32);
void FUN_0803c2c4(void)
{
    u32 position = (u32)gRam02000F08->node - 36;
    s32 index = (position - (u32)gRam02000F08) >> 3;
    s32 start = index;
    s32 passes = 0;
    if (((Phase1SlotHead *)gRam02000F10)->kind == 2 && (gRam02000F08->flags & 2)) return;
    FUN_080384f0();
    do {
        if (index == start && ++passes > 1) return;
        index--;
        if (index < 0) index = 14;
    } while (!FUN_0803c320(index,gRam02000F00));
}
