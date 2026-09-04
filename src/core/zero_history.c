/* Gecmis tamponunu sifirlama — 0x08008108-0x0800811B
 *
 * gHistory'nin 128 baytini Memset ile sifirliyor.
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/zero_history.c
 */

#include "gba_types.h"

#define HISTORY_SIZE 128

extern u8 gHistory[];

extern void Memset(void *dest, u32 value, u32 size);

/* 0x08008108 */
void ZeroHistory(void)
{
    Memset(gHistory, 0, HISTORY_SIZE);
}
