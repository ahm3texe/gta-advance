/* Run the countdown until it goes negative — 0x0805A50C-0x0805A55D
 *
 * NOT BYTE-MATCHING. 37 of 38 instructions, and the difference is two bytes of
 * ORDER in the prologue: the ROM normalises the u16 operand and THEN copies the
 * object into its callee-saved register, and agbcc emits the copy first.
 * Everything else -- the loop shape, the two calls, the address local, the
 * signed division -- is reproduced.
 *
 * Six spellings were measured: the two locals in either declaration order, the
 * multiply split from the divide, an extra u32 holding the operand, the product
 * written with the operands reversed, and the result variable declared first.
 * Five give 37/38 and the other two are worse, so the order is not reachable
 * from the source here.
 *
 * The operand is a percentage: `percent * 60 / 100` is the value the countdown
 * is reloaded with, computed with a SIGNED division.
 *
 * Each turn: if bit 0 of the +0x0C byte is set the countdown is reloaded, the
 * byte cleared and FUN_08063B38(0) run; then the countdown drops by 4, and a
 * negative one answers 1. Otherwise FUN_08063B38(1) decides whether to go
 * round again. Either callee answering 0 ends the loop with 0.
 *
 * The +0x10 field is reached BOTH ways in the ROM: written through the object
 * (`str r6,[r4,#16]`) and read and written through a pointer held in r5
 * (`ldr r0,[r5,#0]`). That is rule 64 -- a separate address local for the
 * countdown, set up once before the loop, next to the object it belongs to.
 *
 * Rule 33 for the flag test: the ROM materialises the 1 first and ands the byte
 * into it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_countdown_loop.c
 */

#include "gba_types.h"

#define PER_UNIT  60
#define PERCENT   100
#define STEP      4

typedef struct Countdown {
    u8  pad00[12];
    u8  reload;                 /* +0x0C */
    u8  pad0D[3];
    s32 left;                   /* +0x10 */
} Countdown;

extern u32 FUN_08063b38(u32 mode);

/* 0x0805A50C */
u32 FUN_0805a50c(Countdown *state, u16 percent)
{
    s32 *left = &state->left;
    s32 value = percent * PER_UNIT / PERCENT;
    u32 probe;

    for (;;) {
        probe = 1;
        probe &= state->reload;
        if (probe != 0) {
            state->left = value;
            state->reload = 0;
            if (FUN_08063b38(0) == 0)
                return 0;
        }
        *left -= STEP;
        if (*left < 0)
            return 1;
        if (FUN_08063b38(1) == 0)
            return 0;
    }
}
