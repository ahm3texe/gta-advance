/* gRam02030330 alan erisimi — 0x080509DC-0x080509ED
 *
 * `base` (+0x10) okuyucu ve `slot` (+0x2C) yazici.
 * Ayni kumedeki CallWithOffset ayri dosyada (henuz esleşmiyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/gRam02030330_gets.c
 */

#include "gba_types.h"

typedef struct Anchor {
    u8  pad00[0x10];
    u32 base;                   /* +0x10 */
    u8  pad14[0x18];
    u32 slot;                   /* +0x2C */
} Anchor;

extern Anchor gRam02030330;

/* 0x080509DC */
u32 GetBase(void)
{
    return gRam02030330.base;
}

/* 0x080509E8 */
void SetSlot(u32 value)
{
    gRam02030330.slot = value;
}
