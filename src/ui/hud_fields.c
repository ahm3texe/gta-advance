/* HUD field writers — 0x08030B60, 0x08030E78, 0x08030F28
 *
 * 0x08030B60 clamps two values to 0..99 and 0..59 and stores them at
 * gRam02025810 +28/+29, suggesting minutes:seconds. The symbol is used as
 * a BASE despite the historical eight-byte ram_map record; the same pattern
 * appears in menu_screen.c (+20), counter_saturate.c (+20 -> +12) and
 * halves_equal.c (+4/+28). extern u8 gRam02025810[] in ram_symbols.h supports
 * this shared view.
 *
 * 0x08030E78/0x08030F28 write tile 0xF0E8 to two VRAM columns. E78 writes
 * and advances one column, then the other; F28 writes BOTH, then advances
 * both. The order comes from source; one form does not produce both.
 *
 * STATUS (2026-09-06): ALL THREE BYTE-MATCHING (40/40/40). E78/F28 were
 * previously parked at 36/40 with a diagnosis of inaccessible allocation.
 * That diagnosis was WRONG; three measured mechanisms solved it.
 *
 * MECHANISM 1 — origin of the extra adds rX,rY,#0 copy:
 *   u16 tile=0xF0E8; *a=tile: PROMOTE_MODE promotes the local to SImode.
 *   Store source becomes (subreg:HI (reg:SI tile)); CSE folds the subreg
 *   directly into the store. One ldr r3,=0xf0e8, total 36 bytes.
 *   *a=0xF0E8: direct HImode constant uses *movhi_insn, which loads the
 *   large constant through a scratch register then copies it:
 *   ldr r0,=0xf0e8 + adds r2,r0,#0, exactly the ROM's extra instruction.
 * ROM scan: all 33 loads of 0xF0E8 immediately copy the register (adds or
 * high-register mov). None uses the constant directly. The same signature
 * occurs at 0x080311DC, 0x0803157C, 0x08031594, 0x08030DE6, 0x08031266.
 *
 * MECHANISM 2 — increasing source counters, another side of rule 42:
 * loop.c hoists the invariant HImode constant to the END of the preheader,
 * after source instructions. A source countdown for (i=7; i>=0; i--) puts
 * movs r0,#7 BEFORE it, preventing scratch use of r0 and requiring r4 plus
 * push {r4,lr}: 44 bytes. Increasing for (i=0; i<8; i++) lets the compiler
 * generate a reversed loop and initialize its counter AFTER hoisting.
 * Scratch/counter r0 do not conflict, giving a stack-free 40-byte leaf:
 *   ldr r0,=0xf0e8 / adds r2,r0,#0 / movs r0,#7.
 *
 * MECHANISM 3 — E78's final five bytes, lifetime-extending no-op (rule 50):
 * After the first two changes, size is 40/40 but a/tile are reversed:
 * ours a=r2,tile=r3; ROM a=r3,tile=r2. Measured allocation:
 *   counter refs7 lifetime18 priority0.778 -> r0
 *   b       refs7 lifetime22 priority0.636 -> r1
 *   a       refs7 lifetime24 priority0.583 -> r2 (fourth in ROM)
 *   tile    refs5 lifetime18 priority0.556 -> r3 (third in ROM)
 * Tile refs are fixed at five: one outside definition plus 2*2 loop uses.
 * References are depth-weighted; each loop use counts twice. More refs
 * require another instruction. Extending a's lifetime to 26 gives
 * 14/26=0.538 <0.556 and fixes order. Lifetime counts instructions from
 * definition to function end, weighted by two. Insert an instruction between
 * a and tile definitions that disappears later.
 *
 * a++; a--; does exactly this: both exist during global allocation, extending
 * a's lifetime 24 -> 32, then vanish in late passes. Differences 5 -> 0;
 * output remains 40 bytes with NO added instruction. Equivalent tested forms:
 * a+=1; a-=1; | a+=2; a-=2; | a=&a[1]; a=&a[-1]; | a=COL_A_LEFT+1; a--.
 * a=a+0 folds in the front end and leaves five differences. Applying the
 * pair to b moves in the wrong direction and gives 7-8 differences.
 *
 * F28 ARRAY INDEXING: a[i]/b[i] become derived induction variables (giv).
 * Their source ldr definitions die; giv initialization follows the hoisted
 * constant, producing pool order tile/right/left and body order two stores,
 * then two increments. E78's interleaved stores/increments keep pointers as
 * source variables (*a=...; a++), whose loads precede the constant.
 * Rule 49: do not copy a sibling's loop form.
 *
 * EARLIER REJECTIONS:
 * - 144 declaration/assignment orders (4!*3!): all 36 bytes.
 * - 13 flag sets including O0/O1/O2/O3/Os, -fno-omit-frame-pointer,
 *   -fforce-mem, -fforce-addr, -fno-strength-reduce, -fno-defer-pop,
 *   -fcaller-saves, -fno-cse-follow-jumps: none changed 36.
 * - for/while/do-while; *p++ vs *p=t;p++; tile types u16/s32/int/vu16;
 *   intermediate copy, counter variable, q=p+32: all 36. All kept tile
 *   in a LOCAL, the actual cause (mechanism 1); the rejection record misled.
 * - agbcc rather than old_agbcc gives 40 only by adding unnecessary
 *   push {lr} / pop {r0}; bx r0; body unchanged, so rejected.
 * THIS SESSION:
 * - Single-field struct/union to force HImode: insv (and+orr), 48 bytes.
 * - t1=C; t2=C: copy appears, but two live locals require r4/push, 44 bytes.
 * - t1=C; t2=t1: combine merges 2->1 and deletes the copy, 36 bytes; a copy
 *   whose source dies is always eliminated here.
 * - *p=t; p[32]=t; p++: no giv, loop add r0,r1,#0 / add r0,#0x40,
 *   36 bytes and two pool words.
 * - u32/s16 counters, i!=8: 44-56 bytes from masking/push.
 * - Chained *a=*b=C adds a volatile readback: 48 bytes.
 * - Assign tile=C inside the loop, then store: still SImode, no copy, 36 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/hud_fields.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define MINUTES_MAX 99
#define SECONDS_MAX 59

#define TILE_BLANK  0xF0E8

/* VRAM tilemap columns written by the two functions. */
#define COL_A_LEFT  ((vu16 *)0x06009AAE)
#define COL_A_RIGHT ((vu16 *)0x06009AEE)
#define COL_B_LEFT  ((vu16 *)0x060098D2)
#define COL_B_RIGHT ((vu16 *)0x06009912)

/* 0x08030B60 */
void SetHudTime(s32 minutes, s32 seconds)
{
    if (minutes < 0)
        minutes = 0;
    if (seconds < 0)
        seconds = 0;
    if (minutes > MINUTES_MAX)
        minutes = MINUTES_MAX;
    if (seconds > SECONDS_MAX)
        seconds = SECONDS_MAX;

    gRam02025810[28] = minutes;
    gRam02025810[29] = seconds;
}

/* 0x08030E78 */
void ClearHudFieldA(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = COL_A_LEFT;
    /* Lifetime-extending no-op pair: no output instructions, but a's lifetime
 * grows 24 -> 32 during global allocation, lowering priority 0.583 -> 0.438.
 * Tile takes r2 and a takes r3 as in the ROM; see mechanism 3 above.
 */
    a++;
    a--;
    b = COL_A_RIGHT;
    /* Store the tile constant DIRECTLY. A u16 local promotes to SImode,
 * leaving one ldr; only the HImode store constant creates adds r2,r0,#0.
 * Use an INCREASING counter: a source countdown initializes before the
 * hoisted constant, forcing scratch r4 and a push (44 bytes).
 */
    for (i = 0; i < 8; i++) {
        *a = TILE_BLANK;
        a++;
        *b = TILE_BLANK;
        b++;
    }
}

/* 0x08030F28 */
void ClearHudFieldB(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = COL_B_LEFT;
    b = COL_B_RIGHT;
    /* Write both columns, THEN advance both (unlike E78). Array indexing
 * creates two giv pointers initialized after the hoisted tile constant,
 * producing the ROM's pool order.
 */
    for (i = 0; i < 6; i++) {
        a[i] = TILE_BLANK;
        b[i] = TILE_BLANK;
    }
}
