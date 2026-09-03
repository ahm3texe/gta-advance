/* Ic ic zincir okuma — 0x08028FFC-0x08029013
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/scan_active.c
 */

#include "gba_types.h"


typedef struct Sub2 { u8 pad00[96]; s8 value; } Sub2;
typedef struct Sub1 { u8 pad00[16]; Sub2 *sub2; } Sub1;
typedef struct Outer { u8 pad00[436]; Sub1 *sub1; } Outer;
typedef struct Arg { u8 pad00[20]; Outer *outer; } Arg;

/* 0x08028FFC */
s32 GetNegatedNested(Arg *a)
{
    return -a->outer->sub1->sub2->value;
}
