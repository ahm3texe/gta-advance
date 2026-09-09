/* Inner-object ID — 0x0803824C-0x0803825F
 *
 * Follow two links. Return zero if either is missing; otherwise return the
 * second object's +0x18 field.
 *
 * BYTE-MATCHING. An explicit early return 0 for the first null check places
 * the shared zero block before the value block, as in the ROM.
 */

#include "gba_types.h"

typedef struct Inner {
    u8  pad00[24];
    u16 id;                     /* +0x18 */
    u8  pad1A[0x12];
    struct Inner *next;         /* +0x2C */
} Inner;

typedef struct Object {
    u8     pad00[0x2C];
    Inner *inner;               /* +0x2C */
} Object;

/* 0x0803824C */
u32 GetInnerId(Object *object)
{
    Inner *inner;

    inner = object->inner;
    if (inner == 0)
        return 0;

    inner = inner->next;
    if (inner != 0)
        return inner->id;

    return 0;
}
