/* Nesne deger cozumu — 0x08038260-0x08038295
 *
 * Once bir sinama nesnesine bakiyor; sonucu sifir degilse onu 16 bit
 * kaydirip donduruyor. Degilse nesnenin turu 4 ise yuva dizisinden
 * dolayli, degilse ayrintidan dogrudan okuyor.
 *
 * Ayni kumedeki diger uc fonksiyon: src/world/object_query.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/object_value.c
 */

#include "gba_types.h"

#define KIND_INDIRECT   4
#define SLOT_OFFSET     436         /* 218 << 1 */
#define BYTE_TO_FIXED   16

typedef struct SlotInner {
    u8  pad00[0x40];
    u32 value;                  /* +0x40 */
} SlotInner;

typedef struct Slot {
    u8         pad00[0x10];
    SlotInner *inner;           /* +0x10 */
} Slot;

typedef struct Detail {
    u8  pad00[0x1C];
    u32 fallback;               /* +0x1C */
} Detail;

typedef struct Object {
    u8      pad00[8];
    u8      kind;               /* +0x08 */
    u8      pad09[7];
    Detail *detail;             /* +0x10 */
    Slot  **slots;              /* +0x14 */
} Object;

typedef struct Probe {
    u8 pad00[8];
    u8 result;                  /* +0x08 */
} Probe;

extern Probe *FUN_08067c10(Object *object);

/* 0x08038260 */
u32 ResolveObjectValue(Object *object)
{
    Probe *probe;

    probe = FUN_08067c10(object);
    if (probe->result != 0)
        return probe->result << BYTE_TO_FIXED;

    if (object->kind == KIND_INDIRECT)
        return (*(Slot **)((u8 *)object->slots + SLOT_OFFSET))->inner->value;

    return object->detail->fallback;
}
