/* Place a pair, and settle if nothing is held — 0x0805A928-0x0805A973
 *
 * The same tail as src/script/cmd_place_record.c without the record lookup:
 * both operands go straight to FUN_0802B394.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_place_pair.c
 */

#include "gba_types.h"

#define HANDLE_A  0x1358

extern u32 gFrameCounterEwram;
extern u8  gRam02025810[];

extern void FUN_0802b394(u16 first, u16 second, u32 mode);
extern void FUN_08029918(void);

/* 0x0805A928 */
u32 FUN_0805a928(u32 a, u16 first, u16 second)
{
    u8 *base;
    u32 offset;

    if (gFrameCounterEwram == 0)
        return 0;
    FUN_0802b394(first, second, 1);
    base = gRam02025810;
    offset = HANDLE_A;
    if (*(u32 *)(base + offset) == 0) {
        offset += 8;
        if (*(u32 *)(base + offset) == 0)
            FUN_08029918();
    }
    return 1;
}
