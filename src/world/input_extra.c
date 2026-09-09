/* Link cable waiting screen — 0x0806620C-0x0806634B
 *
 * Draws two lines of text and enters two separate wait loops. The first loop
 * waits for VBlank on every frame and watches bit 1 (B) of KEYINPUT: once the
 * bit is first seen SET (button RELEASED), bit 0 (A) is written into armed;
 * then when the bit goes CLEAR (B pressed) bit 0 of gBiosIrqFlags is cleared
 * and it returns 1. If A is still not pressed the loop keeps going, otherwise
 * it moves on to the second screen.
 *
 * The second loop runs the same button logic after ResetLinkSession, but on
 * every turn it steps the drivers with FUN_080657d8(0) and repeats inline the
 * same condition as MaybeAdvance (gVBlankEnabled == 2 && gRam0200048C > 1);
 * when the condition holds the loop ends and it returns 0. On the cancel path
 * gBiosIrqFlags is masked, ResetLinkHardware is called, the session state is
 * cleared and it returns 1.
 *
 * STATUS: PARKED — 12/320 bytes off (96.3%). The remaining difference is a
 * SINGLE CLASS: the ORDER of the invariant-expression hoists done BEFORE the
 * loop (in the preheader). Every emitted instruction is the same, and so are
 * the register assignments; in just two places two instructions (and the
 * literal pool words that go with them) are in reversed order:
 *
 *   0x08066238  ROM: mov r6,#2 / ldr r7,=gBiosIrqFlags / mov r4,#1
 *               us:  mov r6,#2 / mov r4,#1 / ldr r7,=gBiosIrqFlags
 *   0x080662C4  ROM: ldr r7,=gVBlankEnabled / ldr r6,=gBiosIrqFlags
 *               us:  ldr r6,=gBiosIrqFlags / ldr r7,=gVBlankEnabled
 *               (the pool words swap places in the same order too)
 *
 * MEASURED MECHANISM: the hoist order follows the order of USE in the source
 * text; but the source arrangement is what forces ROM's block layout, and the
 * two conflict:
 *   - `if (armed == 0) {B} else {A}` -> ROM's block layout is CORRECT
 *     (cmp/bne A, B in the middle, A at the end) but the order is 2,1,flags.
 *   - `if (armed != 0) {A} else {B}` -> the order is 2,flags,1 (ROM) but the
 *     compiler moves B above the loop and adds an entry branch: 316 bytes,
 *     the layout breaks.
 *
 * TRIED AND REJECTED (all measured):
 *   - For loop 2, `while (again)` + `if (!again) break` (the WaitLinkSettle
 *     form): an extra `cmp/beq` remains, and because `again` lives across the
 *     call it holds a callee-saved register -> 328 bytes, 124 off.
 *   - Entering loop 2's do/while with a `goto`: correct tail, but because the
 *     loop is not natural the address hoists disappear entirely (324/235).
 *   - Wrapping loop 2's body with `if (again != 0) {...} continue; break;`:
 *     316, the rotation disappears.
 *   - For loop 1, bfirst/bfirst+continue/bfirst+goto/else-if: all four
 *     produce the same pair (12).
 *   - `gBiosIrqFlags = gBiosIrqFlags & 0xFFFE`, `&= ~1`, armed u32/u16,
 *     again u32, held s32, moving the constant to the left, the order of the
 *     extern declarations: none of them change the order (12).
 *   - Hoisting BY HAND with a pointer/local variable (`bit1 = 2; irq =
 *     &gBiosIrqFlags;`) makes loop 1 match EXACTLY (it drops to 8 bytes):
 *     the initialisation of source-level variables is emitted BEFORE loop.c's
 *     hoists. The same trick fails in loop 2: because of the register that
 *     frees up, the constant 2 gets hoisted as well and a spill into r8
 *     appears (336/344). REJECTED because it is a made-up variable — rather
 *     than writing indefensible source for 8 bytes, 12 bytes with clean
 *     source.
 *
 * RULE 54-61 KIND (all measured, none of them improved anything):
 *   - Writing loop 1 A-first + with `continue`: 316 bytes (the very same
 *     layout breakage that rule 58 documents).
 *   - Rule 54 (splitting the record into two locals): splitting `armed` into
 *     two separate locals (armed/armed2) goes 12 -> 17 bytes, worse. A single
 *     variable is the right call here: both loops want the same register (r5).
 *   - Writing loop 2's condition with a nested `if` (rule 60 form): 12, no
 *     change.
 *   - Rule 57: writing `gBiosIrqFlags` through a volatile view goes 12 -> 24
 *     bytes. Making `gVBlankEnabled` volatile: 12, no change.
 *
 * PERMUTER RUN (1222 iterations): base 80 -> best 20, no zero. The best
 * candidate was REJECTED: it calls VBlankIntrWait twice (a behaviour change),
 * reuses the `held` variable for an unrelated value, and adds a
 * `do{...}while(0)` wrapper.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/input_extra.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define MSG_LINK_WAIT_1   0x1C9
#define MSG_LINK_WAIT_2   0x1CA
#define MSG_LINK_BUSY_1   0x1C6
#define MSG_LINK_BUSY_2   0x1C8

#define TEXT_X            120
#define TEXT_Y_2          32

#define KEY_BIT1          2
#define KEY_BIT0          1
#define BIOS_IRQ_KEEP     0xFFFE
#define LINK_STATE_LIVE   2

extern u16 gVBlankEnabled;
extern u16 gRam0200048C;

extern void FUN_08006124(void);
extern u32  GetTextString(u32 index);
extern void DrawTextCentred(u32 text, s32 x, s32 y);
extern void VBlankIntrWait(void);
extern void ResetLinkSession(void);
extern void ResetLinkHardware(void);
extern void FUN_080657d8(s32 code);

/* 0x0806620C */
s32 WaitForPartner(void)
{
    s32 armed;
    s32 again;
    u16 held;

    armed = 0;

    FUN_08006124();
    DrawTextCentred(GetTextString(MSG_LINK_WAIT_1), TEXT_X, 0);
    DrawTextCentred(GetTextString(MSG_LINK_WAIT_2), TEXT_X, TEXT_Y_2);
    VBlankIntrWait();

    for (;;) {
        VBlankIntrWait();

        if (armed == 0) {
            if (REG_KEYINPUT & KEY_BIT1)
                armed = REG_KEYINPUT & KEY_BIT0;
        } else {
            if ((REG_KEYINPUT & KEY_BIT1) == 0) {
                gBiosIrqFlags &= BIOS_IRQ_KEEP;
                return 1;
            }
            if ((REG_KEYINPUT & KEY_BIT0) == 0)
                break;
        }
    }

    VBlankIntrWait();
    FUN_08006124();
    DrawTextCentred(GetTextString(MSG_LINK_BUSY_1), TEXT_X, 0);
    DrawTextCentred(GetTextString(MSG_LINK_BUSY_2), TEXT_X, TEXT_Y_2);
    ResetLinkSession();
    VBlankIntrWait();

    armed = 0;
    for (;;) {
        FUN_080657d8(0);

        again = 1;
        if (gVBlankEnabled == LINK_STATE_LIVE && gRam0200048C > 1)
            again = 0;
        if (again == 0)
            break;

        if (armed != 0) {
            VBlankIntrWait();
            held = REG_KEYINPUT & KEY_BIT1;
            if (held == 0) {
                gBiosIrqFlags &= BIOS_IRQ_KEEP;
                ResetLinkHardware();
                gVBlankEnabled = held;
                return 1;
            }
        } else {
            if (REG_KEYINPUT & KEY_BIT1)
                armed = REG_KEYINPUT & KEY_BIT0;
        }
    }

    VBlankIntrWait();
    return 0;
}
