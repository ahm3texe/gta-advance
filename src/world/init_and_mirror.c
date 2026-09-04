/* Paketi kopyalayip aynalama — 0x0801D848-0x0801D869
 *
 * Cagiranin 12 baytini nesnenin basina kopyaliyor, sonra nesnenin ilk 116
 * baytini +0x13C'ye aynaliyor ve FUN_0801A560'i cagiriyor.
 *
 * Kural 32: struct atamasi ldmia/stmia cifti uretiyor.
 * ROM 0x13C ofsetini `movs r1,#158 / lsls r1,#1` ile kuruyor (316 ani
 * degere sigmiyor); duz sabit yazmak havuz yuklemesi uretirdi.
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/init_and_mirror.c
 */

#include "gba_types.h"

#define MIRROR_OFFSET (158 << 1)
#define MIRROR_SIZE   116

typedef struct Pack12 {
    u32 a;
    u32 b;
    u32 c;
} Pack12;

extern void FUN_0806dbd0(u8 *dest, u8 *src, u32 size);
extern void FUN_0801a560(void *obj);

/* 0x0801D848 */
void InitAndMirror(u8 *obj, Pack12 *src)
{
    *(Pack12 *)obj = *src;
    FUN_0806dbd0(obj + MIRROR_OFFSET, obj, MIRROR_SIZE);
    FUN_0801a560(obj);
}
