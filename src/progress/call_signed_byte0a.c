/* Hand the signed +0x0A byte to FUN_080504B4 — 0x08031004-0x0803101B
 *
 * The field is read as a plain byte and then sign-extended
 * (`ldrb / lsls #24 / asrs #24`), not with `ldrsb`. That is the shape of a u8
 * field cast to s8 at the point of use, and it is what tells the two apart:
 * src/progress/read_signed_pair.c reads fields that ARE signed, and gets
 * `ldrsb` instead.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/call_signed_byte0a.c
 */

#include "gba_types.h"

extern u8 gRam02025810[];

extern void FUN_080504b4(s32 value);

/* 0x08031004 */
void FUN_08031004(void)
{
    FUN_080504b4((s8)gRam02025810[10]);
}
