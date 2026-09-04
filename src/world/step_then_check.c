/* Adim atip bayrak sinama — 0x08031DB8-0x08031DDB
 *
 * Once FUN_0802EF3C cagriliyor, sonra gRam02025810+0x1358'deki soz sifir
 * degilse FUN_08029C20 cagriliyor. ROM taban ile 0x1358 ofsetini AYRI
 * yukleyip topluyor (ofset sekiz bitlik ani degere sigmiyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/step_then_check.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

typedef struct Progress {
    u8  pad0000[0x1358];
    u32 pending;                /* +0x1358 */
} Progress;

extern void FUN_0802ef3c(void);
extern void FUN_08029c20(void);

/* 0x08031DB8 */
void StepThenCheck(void)
{
    FUN_0802ef3c();
    if (((Progress *)gRam02025810)->pending != 0)
        FUN_08029c20();
}
