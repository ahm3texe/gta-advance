/* gSaveBuffer +0x66 doyuran sayac — 0x0806720C-0x08067227
 *
 * u16 sayaci artiriyor; sonuc sarip sifir olursa ESKI degeri geri yaziyor,
 * yani 0xFFFF'te doyuyor. ROM tasmayi `lsls r0,r0,#16` ile dusuk 16 biti
 * sinayarak olcuyor.
 *
 * SaveBuffer tanimi src/world/copy_flag_byte.c ile BIREBIR AYNI tutulmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bump_save_counter.c
 */

#include "gba_types.h"

typedef struct SaveBuffer {
    u8 pad00[8];
    u8 byte8;                   /* +0x08 */
    u8 byte9;                   /* +0x09 */
    u8 pad0A[92];
    u16 counter66;              /* +0x66 */
    u8 pad68[14];
    u16 counter76;              /* +0x76 */
} SaveBuffer;

extern SaveBuffer gSaveBuffer;

/* 0x0806720C */
void BumpSaveCounter(void)
{
    u16 old;

    old = gSaveBuffer.counter66;
    gSaveBuffer.counter66 = old + 1;
    if ((u16)(old + 1) == 0)
        gSaveBuffer.counter66 = old;
}
