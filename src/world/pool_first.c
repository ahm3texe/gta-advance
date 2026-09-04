/* Havuz +0 getirici — 0x08037F70-0x08037F7B
 *
 * gRam0202F300 havuzunun ILK alanini dondurur. Ayni sembolun +8 alanini
 * donduren dort kardesi src/world/pool_gets.c icindedir.
 *
 * NEDEN AYRI DOSYA: bu fonksiyonu pool_gets.c'ye eklemek, ayni derleme
 * biriminin ORTAK literal havuzunu kaydirip zaten eslesen MarkAndClear'in
 * `ldr [pc,#imm]` degerini bozdu (34/34 -> 1/34). Bitisik olmayan adres
 * kumeleri ayri dosyada tutulmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/pool_first.c
 */

#include "gba_types.h"

typedef struct Pool {
    u32 first;                  /* +0x00 */
} Pool;

extern Pool gRam0202F300;

/* 0x08037F70 */
u32 GetPoolBFirst(void) { return gRam0202F300.first; }
