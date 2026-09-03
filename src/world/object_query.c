/* Nesne sorgulari — 0x080381F8-0x0803825F
 *
 * Iki fonksiyon. Ilki ROM'daki sabit bir dort sozcuklu yapiyi yerele
 * kopyalayip (ldmia/stmia cifti struct atamasindan geliyor) sinama
 * fonksiyonuna veriyor.
 *
 * DURUM: SubmitObject esleşiyor, ProbeObject 5 bayt ve GetInnerId 4 bayt
 * farkli. Ikisinde de fark ayni: ROM `return 0` blogunu ONE, deger
 * donusunu SONA koyuyor; ProbeObject'te literal havuzu tam bu iki blogun
 * ARASINDA duruyor. Havuz yerlesimi C'den denetlenemedigi icin blok sirasi
 * da denetlenemiyor.
 *
 * Denenenler (hepsi birebir ayni sonucu verdi -- agbcc bu bicimlere hic
 * duyarli degil): duz erken donus, ic ice if, cagri sonucunu yerele almak,
 * alan okumasini yerele almak. Sonuc yereli ise cok kotulestirdi (38/17).
 *
 * Bu kumeden esleşen ResolveObjectValue ayri dosyada:
 * src/world/object_value.c
 *
 * Ayni kumeden eslesen ikisi ayri dosyada: SubmitObject ->
 * src/world/submit_object.c, ResolveObjectValue -> object_value.c
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

typedef struct Inner {
    u8  pad00[24];
    u16 id;                     /* +0x18 */
    u8  pad1A[0x12];
    struct Inner *next;         /* +0x2C */
} Inner;

typedef struct Object {
    u8      pad00[8];
    u8      kind;               /* +0x08 */
    u8      pad09[7];
    u8      pad10[4];
    u8      pad14[4];
    u8      pad18[0x10];
    u32    *target;             /* +0x28 */
    Inner  *inner;              /* +0x2C */
} Object;

extern u32    FUN_0804293c(u32 *target, const Params *params);

/* 0x080381F8 */
u32 ProbeObject(Object *object)
{
    Params params;

    params = DEFAULT_PARAMS;

    if (object != 0) {
        if (object->target != 0) {
            if (FUN_0804293c(object->target, &params) != 0)
                return 1;
        }
    }

    return 0;
}

/* 0x0803824C */
u32 GetInnerId(Object *object)
{
    Inner *inner;

    inner = object->inner;
    if (inner != 0) {
        inner = inner->next;
        if (inner != 0)
            return inner->id;
    }

    return 0;
}
