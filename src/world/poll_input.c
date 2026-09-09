/* Reading the keys and the input history — 0x080656F4-0x080657D7
 *
 * Inverts KEYINPUT and extracts the 10-bit pressed-key mask.  If all four
 * directions are pressed while the shoulder buttons are NOT, it drops into a
 * hardware shutdown (the soft-reset shortcut).
 *
 * While the link state is 2, the input is written into a 32-element ring
 * buffer every frame; if the input differs from the previous frame a counter
 * is bumped and the "last change" index is recorded.  If the difference
 * between the indices exceeds 12 the history window is slid forward.  Two side
 * buffers also store that frame's counter and window values.
 *
 * The ring buffer sizes were MEASURED from the ROM: the Memset calls at
 * 0x08066144 give 64 (32 x u16) and 32 (32 x u8), and the indexing is `& 31`.
 *
 * STATUS: PARKED — 224/228 bytes, 4 short.  Three levers were found and
 * applied:
 *
 *  1. The two key tests must be SEPARATE `if`s.  In a single `&&` expression
 *     GCC folds the two into one mask (`0x30F`) because 0x300 and 0x00F are
 *     disjoint, and that does not agree with the ROM's two separate branches.
 *  2. `index` must be s32.  Declared u16, agbcc adds a 16-bit truncation
 *     (`lsls #16 / lsrs #16`) at every assignment; the ROM does not truncate.
 *  3. The byte narrowing must be `& 0xFF`, not a `(u8)` cast.  The cast
 *     produces a 24-bit shift pair and consumes an extra callee-saved register
 *     (r8) -- measured, dump_alloc pseudos 119/120/121.
 * These three steps took the difference from 185 to 4 bytes.
 *
 * THE REMAINING DIFFERENCE (91 ROM instructions against our 90; 2 bytes of
 * instruction + 2 bytes of pool alignment):
 *   a) In the shoulder mask test the ROM COPIES both operands into separate
 *      pseudos (`adds r1,r0,#0 / adds r0,r4,#0 / ands r0,r1`) while we do it
 *      in one instruction (`ands r0,r4`).  (The ROM has +2 instructions.)
 *   b) At the `strb` right after the counter bump the ROM still keeps the
 *      address register (r1) live and uses it directly, while we reload it
 *      from ip with `mov r1,ip`.  (We have +1 instruction.)
 * (b) is pure allocation: the ROM keeps the array base in r1 and the index
 * temporary in r0; we do the reverse.
 *
 * (a)'S MECHANISM WAS MEASURED FROM AN RTL DUMP (2026-09-07,
 * `old_agbcc -da`): combine produces
 * `(set (reg 37) (and (reg/v 22) (reg 36)))` and reg 36 (the 0x300 constant)
 * DIES there (REG_DEAD).  regmove's `fixup_match_1` therefore renames the
 * destination 37 -> 36 and a single instruction comes out.  The ROM's
 * three-instruction form only arises when NO operand dies (or when
 * `reg_is_remote_constant_p` fires, i.e. when the constant's `set` is in
 * ANOTHER basic block).  `keys` (reg 22) already does not die; the only
 * remaining lever is for the constant not to die either, i.e. a second use of
 * 0x300 in the function -- and there is NO such use in the ROM.  No
 * source-level lever was found.
 * NOTE: rule 59's `u8` local DOES NOT APPLY HERE; 0x300 is not in the low
 * byte, and a `u8` local clips the mask and breaks the meaning (measured: 216
 * bytes).
 *
 * THE PERMUTER WAS RUN (1296 iterations): 875 -> 425, it did not reach zero.
 * The best candidate makes `index` an `unsigned long long`; it lowers the
 * score but cannot be the real source for a u16 counter, so it was REJECTED.
 *
 * SPELLINGS RULED OUT (all 224 bytes, no change; 2026-09-07):
 * taking the mask result into a u16/u32/s32 local; into a `u8` local (216,
 * meaning broken); `!(keys & MASK)`; a `mask = KEY_SHOULDERS;` intermediate; a
 * single `&&` expression; `MASK & keys` (rule 53);
 * `shoulders = keys; shoulders &= MASK;` (u16/u32/s32); `if (... > 0) ; else`;
 * an empty `else` branch; computing the shoulder and dpad masks together
 * first.  Making `keys` u32/s32/int LOWERS it to 220 bytes (worse).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/poll_input.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "game_state.h"

#define KEY_MASK        0x3FF
#define KEY_SHOULDERS   0x300
#define KEY_DPAD        0x00F
#define HISTORY_MASK    31
#define WINDOW_SPAN     12
#define LINK_STATE_LIVE 2
#define RESET_ARG       0xFF

extern u16 gVBlankEnabled;
extern u16 gRam0200048C;
extern u16 gRam02000420[];
extern u8  gRam020003C0[];
extern u8  gRam02000E80[];
extern u8  gRam02036320;
extern u8  gRam0203632C;

extern void ShutdownAndReset(void);
extern void FUN_0806b88c(u32 arg);

/* 0x080656F4 */
void PollInput(void)
{
    u16 keys;
    s32 index;

    keys = ~REG_KEYINPUT & KEY_MASK;

    if ((keys & KEY_SHOULDERS) == 0) {
        if ((keys & KEY_DPAD) == KEY_DPAD)
            ShutdownAndReset();
    }

    gGameState.pressed = keys & ~gGameState.half04;
    gGameState.held    = keys;

    if (gVBlankEnabled == LINK_STATE_LIVE) {
        index = gRam0200048C + 1;
        gRam0200048C = index;

        gRam02000420[index & HISTORY_MASK] = keys;

        if (gGameState.held != gRam02000420[(gRam0200048C - 1) & HISTORY_MASK]) {
            gRam02036320++;
            gRam0203632C = index;
        }

        if (((gRam0200048C - gRam0203632C) & 0xFF) > WINDOW_SPAN)
            gRam0203632C = gRam0200048C - WINDOW_SPAN;

        gRam020003C0[gRam0200048C & HISTORY_MASK] = gRam02036320;
        gRam02000E80[gRam0200048C & HISTORY_MASK] = gRam0203632C;
    }

    if ((keys & KEY_DPAD) == KEY_DPAD)
        FUN_0806b88c(RESET_ARG);
}
