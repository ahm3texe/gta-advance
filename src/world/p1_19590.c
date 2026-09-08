/* Bilesik sprite parcalarindan genislik ve yukseklik bul — 0x08019590.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x08019590.json. */
#include "gba_types.h"
#include "phase1_types.h"

extern Phase1SpriteDesc gRom08BD3448;
void FUN_08019590(u32 kind,u32 variant,u32 *width,u32 *height)
{
    Phase1SpriteDesc *desc = gRom08BD3448.slots[kind]->slots[variant]->slots[0];
    u32 w,h;
    if (desc->count == 1) {
        h = desc->pieces[0][1]; w = desc->pieces[0][0];
    } else if (desc->count == 2) {
        h = desc->pieces[1][1] + desc->pieces[0][1]; w = desc->pieces[0][0];
    } else {
        h = desc->pieces[2][1] + desc->pieces[0][1];
        w = desc->pieces[1][0] + desc->pieces[0][0];
    }
    *width = w; *height = h;
}
