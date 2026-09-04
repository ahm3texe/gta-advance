/* Yuva secicileri — 0x0803C49C-0x0803C507
 *
 * Uc kucuk secici. Arguman 1/2'ye gore uc ayri yuva blogundan (0x02000F10,
 * 0x02001140, 0x02001060, 0x02000F80) birini donduruyor. Ucuncusu ayrica
 * gGameState[12] duraklamasi kuruluyken +0x20 alt yapisindan +0x12 u16
 * cikariyor.
 *
 * Isleyici adresleri gibi bu isaretciler de RAM'de tutuluyor; #define yerine
 * extern u32 kullanildi cunku ROM cast/toplama yerine dogrudan `ldr`
 * uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_selectors.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SELECT_A     1
#define SELECT_B     2

typedef struct SubData {
    u8 pad00[0x12];
    u16 value;                  /* +0x12 */
} SubData;

typedef struct SlotHead {
    u8       pad00[0x20];
    SubData *sub;               /* +0x20 */
} SlotHead;

extern u32       gRam02001060;
extern SlotHead  gRam02000F80;
extern u8        gGameState[];

/* 0x0803C49C */
void *SelectSlotAB(u32 which)
{
    if (which == SELECT_A)
        return gRam02000F10;
    if (which == SELECT_B)
        return gRam02001140;

    return 0;
}

/* 0x0803C4B8 */
void *SelectSlotCD(u32 which)
{
    if (which == SELECT_A)
        return &gRam02001060;
    if (which == SELECT_B)
        return &gRam02000F80;

    return 0;
}

/* 0x0803C4D4 */
u16 GetSubValue(u32 which)
{
    SlotHead *head;
    SubData  *sub;

    if (which == SELECT_A) {
        head = (SlotHead *)&gRam02001060;
    } else {
        if (gGameState[12] == 0)
            return 0;
        if (which != SELECT_B)
            return 0;
        head = &gRam02000F80;
    }

    sub = head->sub;
    if (sub == 0)
        return 0;

    return sub->value;
}
