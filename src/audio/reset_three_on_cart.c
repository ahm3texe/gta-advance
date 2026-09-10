/* Clear the three words when the cart flag is 1 — 0x08034980-0x080349A3
 *
 * The same body as src/audio/reset_three_and_flag.c behind a guard on
 * gCartFlag, which data/ram_map.csv records as the flag 0x080336EC tests.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/reset_three_on_cart.c
 */

#include "gba_types.h"

typedef struct AudioState {
    u8  pad00[0x10];
    u32 first;                  /* +0x10 */
    u8  pad14[0x10];
    u32 second;                 /* +0x24 */
    u32 third;                  /* +0x28 */
    u8  busy;                   /* +0x2C */
    u8  pad2D[3];
} AudioState;

extern u8         gCartFlag;
extern AudioState gRam020001B0;

extern void FUN_08033ba0(void);

/* 0x08034980 */
void FUN_08034980(void)
{
    if (gCartFlag != 1)
        return;
    gRam020001B0.first = 0;
    gRam020001B0.second = 0;
    gRam020001B0.third = 0;
    FUN_08033ba0();
}
