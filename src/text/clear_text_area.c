/* Clear a text area — 0x08064590-0x08064613
 *
 * DMA3 clears the text layer starting at the supplied cell. The second block
 * runs only when gHalfLineSpacing is zero (full line height).
 *
 * NOT YET MATCHING: 65 of 66 instructions match, with ONE BYTE different at
 * 0x080645F2: the destination register of an unused DMA-control read.
 *   ROM: ldr r0,[r4,#8]   ours: ldr r3,[r4,#8]
 * The value is discarded; the difference is allocation, not semantics.
 * MEASURED CAUSE (COMPILER.md, register allocation): leaving both reads bare
 * puts control in r4 and dma in r3, opposite to the ROM (13 bytes). A bare
 * read gets r0 but removes control's sixth reference and reverses allocation
 * order. Both requirements could not be met together.
 *
 * Rejected attempts, all worse: separate discard 13; bare expression 13;
 * assign both reads to control 24; assign the dead read to row 13, col 20,
 * dest 13, fill 57, ime 56; alter the first block's read 63-103. All 5040
 * declaration permutations of seven locals were tried; none beat 1. Also:
 * - Reuse height as control, following ROM add r3,r2,#0: 15.
 * - Define dma earlier to extend its lifetime/lower priority: 13/17/56/64
 *   depending on position. The constant load materializes there and shifts
 *   instruction order, with no gain.
 * - Move DMA setup into an inline helper called twice: 55-59.
 * This form is the measured local optimum.
 *
 * PERMUTER: decomp-permuter-agbcc (tools/setup_permuter.sh) failed to beat
 * base score 205 in 13,917 iterations, finding only equal-scoring variants.
 * Evidence then comprised 5040 declaration permutations, 18 targeted manual
 * attempts and 13,917 random attempts. The recorded conclusion was a measured
 * barrier, perhaps requiring the true translation unit or an unmeasured
 * agbcc behavior rather than local source rearrangement.
 *
 * THE REVERSE APPROACH also failed. The remove-locals technique that solved
 * SetBg1Enable gave: remove control 13; remove row/col and inline dest 116;
 * bare read in block two 13; remove dest 111. None approached base 1. Thus
 * 18 local-adding, four local-removing, 13,917 random attempts and 5040
 * declaration permutations had all failed.
 *
 * REFINED DIAGNOSIS after rule 35-38 variants: a bare read plus computing
 * control in THREE statements fixes offset 0x62 exactly (read into r0),
 * but moves the difference to 0x38/0x3a:
 *   ROM: lsl r0,r3,#6 / asr r3,r0,#1 — intermediate through r0
 *   ours: lsl r3,r3,#6 / asr r3,r3,#1 — in place
 * The barrier is preserving control allocation while routing the shift
 * through r0. Two independent routes yield the same two-byte obstruction
 * (three statements = 2; four in-place statements = 2).
 * Also rejected: separate dma2 base for block two (rule 37), 13 because the
 * copy merges; separate shifted local 13 (changes reference balance); reuse
 * dead row 58, col 20, height 13; construct constant first then |=, 60.
 *
 * A forced register volatile DmaChannel *dma asm("r4") reached zero bytes
 * but was REJECTED under WORKFLOW.md §6: it forces matching while hiding
 * the explanation. It shows r4 is reachable; the problem is triggering it
 * with natural C.
 *
 * PERMUTER RESULT (2026-09-05): 24,368 iterations never improved base score 5.
 * The one-byte difference is an allocation artifact unaffected by those local
 * mutations. The recorded assessment was that longer runs would revisit the
 * same plateau. Reproduction setup:
 *   python3 tools/make_permuter_dir.py src/text/clear_text_area.c \
 *       ClearTextArea 0x08064590 132
 *
 * RULE 50 DIAGNOSIS (2026-09-06): a single ordering decision, measured with
 * tools/dump_alloc.py, superseded the earlier dead-load-register diagnosis:
 *   p29 control: refs=5, lifetime=21, priority=0.476 — processed second
 *   p30 dma:     refs=9, lifetime=52, priority=0.519 — processed first
 * Both conflict with {r0,r1,r2}; find_reg gives the first r3 and the next r4.
 * The ROM needs control=r3, dma=r4, so control must be processed FIRST.
 *
 * Two bare reads yield the desired ldr r0 but remove control's sixth reference:
 * priority drops 0.545 -> 0.476, order reverses, and 13 bytes differ. The
 * one-byte form buys reference six by assigning the read to control, at the
 * cost of loading into r3. A sixth reference requires an instruction operand:
 * the ROM's five-instruction definition (lsl/asr/movs/lsl/orr) gives control
 * three references plus two stores = five. No free sixth reference exists.
 * Its lifetime is 21 but must be <=19 to win; definition at asrs and final
 * use at block 3 instructions 9 are fixed by ROM order.
 *
 * TWO INDEPENDENT ROUTES reached the target allocation, each leaving one issue:
 * 1. Define dma after ime = REG_IME: lifetime 52 -> 60, priority 0.450 < 0.476.
 *    control=r3, dma=r4, both dead reads ldr r0: 65/66 instructions match.
 *    But ldr r4,[pc] appears four instructions early (10 bytes). Available
 *    lifetimes are quantized: 52/56/60/68; required 57 is unreachable at a
 *    C insertion point. Four measured positions: 13/18/10/17.
 * 2. Reuse height in place: height <<= 6; height >>= 1; height |= 0x81000000.
 *    refs=9, lifetime=52, priority=0.519 exactly ties dma. Rule 50's lower-
 *    pseudo tie-break lets p24 take r3, then dma r4, matching the ROM. Only
 *    the in-place shifts differ (lsls r3,r3,#6 / asrs r3,r3,#1 instead of
 *    lsls r0,r3,#6 / asrs r3,r0,#1), two bytes. Separating the intermediate
 *    (col = height << 6; height = (col>>1)|K) fixes the shift but drops refs
 *    9 -> 7 and floor_log2 3 -> 2, priority 0.269: 22 differences. Even
 *    eight references give 3*8/52 = 0.4615 < 0.519; exactly nine are needed,
 *    without free eighth/ninth references.
 * A third route, separate height with eight references (3*8/28 = 0.857),
 * would fix order, but the ROM uses height only for its entry copy and lsls:
 * two references, with no six free additions.
 *
 * FURTHER MEASURED REJECTIONS, all above or equal to base 1:
 * - 168 combinations of control definition, block-2 read, CFG and block-3
 *   read target: best 12 results plateau at 1, none zero. Dead-read targets
 *   x/y/height gave 1/2/2; row/col/dest/fill/ime were already eliminated.
 * - Remove dma and use macros for all eight accesses: 13. CSE recreates
 *   one pseudo with refs 9/lifetime 52.
 * - Separate ctl = &dma->control: 13; agbcc folds it into dma+8 with no new allocno.
 * - Macros in block 3, local in block 2: 13, still one pseudo.
 * - CFG changes if (==0), if (!), early return, goto, empty else all leave
 *   refs=5/lifetime=21 and refs=9/lifetime=52 unchanged: 13. do-while(0): 15.
 * - Extend dma through a tail read: refs rise to 10/11, floor_log2 stays 3,
 *   priority rises to 0.536/0.569: 23/44.
 * The recorded conclusion: one byte is the measured optimum for this variable
 * structure; revisit these three routes' numbers before repeating attempts.
 *
 * INTEGER-PRIORITY DIAGNOSIS (2026-09-06): global.c allocno_compare truncates
 * pri = (int)(floor_log2(refs)*refs/lifetime * 10000); lower allocno wins ties.
 *   control p29: 5/21 -> 4761; dma p30: 9/52 -> 5192.
 * Even a tie suffices because p29 < p30; control is 431 points short.
 * All identified routes and their obstructions:
 * (A) control refs 6/lifetime <=23 -> 5217+. Only assigning the dead read
 *     creates reference six, forcing r3. The ROM has exactly five control
 *     references: asr 1, orr 2, two stores 2. No free sixth operand exists.
 * (B) control refs 5/lifetime <=19 -> 5263. Lifetime counts RTL instructions
 *     from definition (orr/asr) to block-3 store, both fixed in the ROM;
 *     the 20-instruction span cannot shrink.
 * (C) dma lifetime >=57 -> 4736. Measured lifetime is twice the RTL span.
 *     Definition positions give 52/56/60/68/86 (13/18/10/17/43 bytes);
 *     57..59 cannot be produced, and each step moves ldr r4,[pc].
 * (D) dma refs 8 -> 4615. Current refs = definition + eight memory accesses;
 *     all eight must use r4 in the ROM, so none can be removed.
 * (E) Another allocno takes r3 before dma. Only p24 (height) and p48
 *     (height<<6 temporary) conflict with dma but not control. p48 must use
 *     r0 for ROM lsls r0,r3,#6. p24 has 2 refs/lifetime 28 -> 714 and would
 *     need eight references to outrank dma; the ROM supplies two. No third candidate.
 *
 * NEW MEASURED REJECTIONS:
 * - Full 243-variant cross product: three control definitions x nine dead-read
 *   targets (bare/control/x/y/height/row/col/dest/ime) in each of two blocks.
 *   Six forms plateau at base 1; none reaches zero.
 * - Add RTL copies hoping reload deletes them: dest2=dest, fp=&fill, dmb=dma,
 *   two-stage copies, cross-block ctl2=control. All six tested forms disappear
 *   BEFORE global allocation; p29/p30 refs/lifetimes stay identical (all 13).
 * - Separate block scopes for fill: 50. Compute control earlier: 34, with
 *   dma lifetime falling 52 -> 44, the wrong direction.
 * - Reverse declarations (dma first): 34, also losing the tie-break advantage.
 *
 * SECOND INDEPENDENT TWO-BYTE ROUTE: compute control in place
 * (control = height << 6; control >>= 1; control |= K). refs=7/lifetime=24
 * -> 5833, giving ROM allocation control=r3/dma=r4 and both dead reads into r0.
 * Only the in-place shifts differ. Every separated-intermediate form tested
 * (f2/f3/f8/f9/f14/f16/f19) drops refs to five and reverses order. Six-plus
 * references require in-place shifting; the correct shift gives only five.
 *
 * The investigation concluded that its source-level search space was exhausted.
 * Before a new attempt, reproduce (A)-(E); each is measurable with one command.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/clear_text_area.c
 * Diagnosis:     python3 tools/dump_alloc.py src/text/clear_text_area.c \
 *                 ClearTextArea --rom --conflicts
 */

#include "gba_io.h"

#define GLYPH_FIRST      32
#define GLYPH_SUBSTITUTE 146
#define GLYPH_APOSTROPHE 39
#define COLOR_ESCAPE    64
#define SCREEN_RIGHT     239
#define SCREEN_BOTTOM    159
#define LINE_HEIGHT      16
#define LINE_HEIGHT_HALF 8
#define TILE_SHIFT       3

extern u32 gTextRowStride;
extern u8 *gTextVramBase;
extern u8 *gGlyphWidths;
extern u32 gFontIndex;
extern u32 gHalfLineSpacing;

extern u8   _toupper(u8 ch);
extern void PlaceGlyph(u8 ch, s32 x, s32 y);

s32 GetGlyphWidth(u32 ch);
s32 GetTextWidth(const u8 *text);

#define IS_COLOR_DIGIT(c) ((u8)((c) - 56) <= 1 || (c) == 48)

static __inline__ s32 GlyphAdvance(u32 ch)
{
    s32 index;
    u32 width;
    s32 advance;

    index = ch;
    if (index == GLYPH_SUBSTITUTE)
        index = GLYPH_APOSTROPHE;

    index = _toupper((u8)index);
    index -= GLYPH_FIRST;
    if (index < 0)
        return 0;

    width = gGlyphWidths[index];
    advance = width + 2;
    if (advance & 1)
        advance = width + 3;

    return advance;
}

static __inline__ s32 DrawTextAt(const u8 *text, s32 x, s32 y)
{
    s32 left;
    u8 ch;

    left = x;

    while ((ch = *text++) != 0) {
        if (ch == COLOR_ESCAPE && IS_COLOR_DIGIT(*text)) {
            text++;
            continue;
        }

        if (ch == 10 || ch == 13) {
            x = left;
            if (gHalfLineSpacing == 0)
                y += LINE_HEIGHT;
            else
                y += LINE_HEIGHT_HALF;
            continue;
        }

        ch = _toupper(ch);
        PlaceGlyph(ch, x, y);
        x += GetGlyphWidth(ch);
        if (x > SCREEN_RIGHT)
            break;
    }

    return x;
}

/* 0x08064590 */
void ClearTextArea(s32 x, s32 y, s32 height)
{
    volatile u16 fill;
    u16 ime;
    u8 *dest;
    s32 row;
    s32 col;
    s32 control;
    volatile DmaChannel *dma;

    if ((u32)x > SCREEN_RIGHT)
        return;
    if ((u32)y > SCREEN_BOTTOM)
        return;

    row = y >> TILE_SHIFT;
    col = x >> TILE_SHIFT;
    dest = gTextVramBase + (col << 6) + (row * gTextRowStride << 6);

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma = (volatile DmaChannel *)REG_DMA3_ADDR;
    dma->src = &fill;
    dma->dst = dest;
    control = ((height << 6) >> 1) | 0x81000000;
    dma->control = control;
    dma->control;
    REG_IME = ime;

    if (gHalfLineSpacing == 0) {
        dest += gTextRowStride << 6;

        ime = REG_IME;
        REG_IME = 0;
        fill = 0;
        dma->src = &fill;
        dma->dst = dest;
        dma->control = control;
        control = dma->control;
        REG_IME = ime;
    }
}
