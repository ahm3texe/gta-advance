/* Resolve an object value — 0x08038260-0x08038295
 *
 * Query a test object first; if nonzero, shift the result by 16 and return it.
 * Otherwise read indirectly from the slot array for type 4, or directly from
 * the detail object for other types.
 *
 * Three other functions in this cluster: src/world/object_query.c
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/object_value.c
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
