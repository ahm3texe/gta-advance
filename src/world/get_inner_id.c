/* Ic nesne kimligi — 0x0803824C-0x0803825F
 *
 * Iki baglantiyi izler; zincirin herhangi bir halkasi yoksa sifir,
 * ikinci halkanin +0x18 alanini aksi halde dondurur.
 *
 * BYTE-MATCHING. Ilk null kontrolunu acik erken `return 0` biciminde
 * yazmak ROM'daki ortak sifir blogunu deger blogundan once yerlestirir.
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
