/* Sayaci artir ya da tetikle — 0x08050108-0x0805012F
 *
 * Alt nesnenin +0xA5 baytini ISARETLI olarak sinayip pozitifse
 * FUN_08055674'u cagiriyor ve 1 donuyor; degilse bayti artirip 0 donuyor.
 *
 * ROM ayni bayti IKI KEZ okuyor: once `ldrb` (artirma icin), sonra
 * `ldrsb` (isaretli sinama icin). Iki farkli tur gerektigi icin kaynakta
 * da iki ayri okuma var.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bump_or_trigger.c
 */

#include "gba_types.h"

#define COUNTER_OFFSET 0xA5

typedef struct Obj {
    u8  pad00[28];
    u8 *sub;                    /* +0x1C */
} Obj;

extern void FUN_08055674(Obj *obj);

/* 0x08050108 */
u32 BumpOrTrigger(Obj *obj)
{
    u8 *counter;
    u8  value;

    counter = obj->sub + COUNTER_OFFSET;
    value = *counter;
    if (*(s8 *)counter > 0) {
        FUN_08055674(obj);
        return 1;
    }
    *counter = value + 1;
    return 0;
}
