/* Nesne gonderme — 0x08038234-0x0803824B
 *
 * FUN_08038608'in donusunu sabit 1 ve cagri argumaniyla FUN_08036cac'a
 * veriyor. Epilog `pop {r1}; bx r1` oldugu icin fonksiyon DEGER
 * DONDURUYOR: r0 canli kaliyor.
 *
 * Kardesleri src/world/object_query.c (henuz eslesmiyor) ve
 * src/world/object_value.c icinde.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/submit_object.c
 */

#include "gba_types.h"

typedef struct Object Object;

extern u32 FUN_08038608(Object *object);
extern u32 FUN_08036cac(u32 handle, int one, u32 arg);

/* 0x08038234 */
u32 SubmitObject(Object *object, u32 arg)
{
    return FUN_08036cac(FUN_08038608(object), 1, arg);
}
