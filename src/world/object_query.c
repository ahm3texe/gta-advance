/* Nesne sorgusu — 0x080381F8-0x08038233
 *
 * ROM'daki sabit bir dort sozcuklu yapiyi yerele
 * kopyalayip (ldmia/stmia cifti struct atamasindan geliyor) sinama
 * fonksiyonuna veriyor.
 *
 * BYTE-MATCHING. Null kontrollerini acik erken `return 0` biciminde yazmak
 * ROM'daki sifir blogunu literal havuzundan once yerlestirir. Ic ice `if`
 * bicimi ayni semantige sahip olsa da blogu havuzdan sonraya tasiyordu.
 *
 * Bitişik iki eslesen fonksiyon ayri dosyalarda: SubmitObject ->
 * src/world/submit_object.c, GetInnerId -> src/world/get_inner_id.c.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/object_query.c
 */

#include "gba_types.h"


typedef struct Params {
    u32 a;
    u32 b;
    u32 c;
    u32 d;
} Params;

#define DEFAULT_PARAMS  (*(const Params *)0x083493D4)

typedef struct Object {
    u8      pad00[8];
    u8      pad08[0x20];
    u32    *target;             /* +0x28 */
} Object;

extern u32    FUN_0804293c(u32 *target, const Params *params);

/* 0x080381F8 */
u32 ProbeObject(Object *object)
{
    Params params;

    params = DEFAULT_PARAMS;

    if (object == 0)
        return 0;
    if (object->target == 0)
        return 0;
    if (FUN_0804293c(object->target, &params) != 0)
        return 1;

    return 0;
}
