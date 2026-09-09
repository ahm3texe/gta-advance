# Band B/C — closed work (history)

**STATUS: BOTH ITEMS CLOSED (2026-09-07).**

- `0x08031844` -> `src/video/blit_strip_clip_left.c`, BYTE-MATCHING.
  The solution: diffing the ROM bodies against its matching sibling
  `0x08031A1C`. 235 of 235 instructions were identical; the only difference was
  the polarity of the column test (`if (col++ >= 0)` -> `if (col++ < 0)`).
  Method in docs/WORKFLOW.md §10.
- The `0x0803173E` boundary error was corrected; its replacements `0x08031684`
  and `0x080316B0` -> `src/video/blit_strip_plain.c`, both BYTE-MATCHING.

The notes below are the agent's own, written BEFORE the solution; they are kept
for the methodological lesson: the register priorities had been measured
correctly, but the WRONG QUESTION was being asked.

---

## 0x08031844 — 472 bytes, 8x48 4bpp strip blitter (NO MATCH)

226/235 instructions identical. The remaining difference was a single
three-register cyclic permutation (`{p0, under, mask}` -> r3/r4/r5) plus one
extra `mov ip,r6`. The `tools/dump_alloc.py` measurement: the accumulator's
priority needed to exceed 2,263 but was 1,751 (62 references / lifetime 177);
what was needed was either lifetime <= 136 or references >= 67, and NO
SOURCE-LEVEL spelling was found that produced either (~40 spellings eliminated).

The agent's in-file note and the working body are below; a new attempt should
start from here.

```c
/* ---- 0x08031844 — 470 bytes, NO MATCH -------------------------------------
 *
 * Builds an 8-pixel-wide, 48-row 4bpp strip.  For each row: if the row number
 * is outside [0, height), it takes the eight source bytes as they are; if it is
 * inside, then for each pixel, a negative column number again takes the source,
 * and otherwise the blend `(under[k] & mask[k]) | src[k]`.  The eight values are
 * packed into four-bit fields and written to `out` as two half-words.  At the
 * end of a row, `mask` advances by a variable stride, and `under` and `src`
 * advance by 40 bytes (the row stride is 48, since the eight pixels have already
 * been advanced one at a time).
 * The parameter names are ROLE GUESSES; the only evidence is the advance steps.
 *
 * STATUS: WITH REGISTER NAMES NORMALIZED, 226 OF 235 INSTRUCTIONS ARE IDENTICAL.
 * The raw measurement is 160/236 instructions, 329/472 bytes; the raw numbers are
 * misleading, because nearly the whole difference comes from ONE register
 * permutation (renaming the ROM's r3/r4/r5 triple to {p0, under, mask} and
 * comparing again leaves only the two items below).
 *
 * THE REMAINING DIFFERENCE IS EXACTLY TWO THINGS:
 *  1) A THREE-REGISTER CYCLIC PERMUTATION.
 *     ROM:  r3 = p0 (pack accumulator), r4 = under (arg5), r5 = mask (arg4)
 *     Ours: r5 = p0,                    r3 = under,        r4 = mask
 *     Global allocator priorities measured with tools/dump_alloc.py:
 *         pseudo 27 (under) refs 89 lifetime 236 priority 2,263  -> r3
 *         pseudo 26 (mask)  refs 89 lifetime 237 priority 2,253  -> r4
 *         pseudo 33 (p0)    refs 62 lifetime 177 priority 1,751  -> r5
 *     For the ROM's allocation, p0's priority must rise ABOVE 2,263.  Since
 *     priority is `floor_log2(refs) * refs / lifetime`, there are two ways:
 *         (a) with refs fixed at 62, lifetime <= 136 (currently 177), or
 *         (b) with lifetime fixed at 177, refs >= 67 (currently 62).
 *     NO source-level lever was FOUND that achieves either.
 *  2) One extra `mov ip, r6` on the skipped-row path (2 bytes; 472 vs 470).
 *     p1's global register is r6 in the ROM and ip in ours; this is a
 *     by-product of (1).
 *
 * SPELLINGS MEASURED AND ELIMINATED (do not retry — none changed (1)):
 *   declaration order: 10 permutations of the p's relative to i/x/y -> all same
 *   types: u8 / s32 / int / u16 for p0..p3 -> all same
 *   scope: declaring p0..p3 inside the loop body -> same
 *   the `register` keyword (on p0, and on all four) -> same
 *   packing: a separate `w` variable, `p0 |= ...` accumulation, reversed-order
 *     OR, pairwise grouping, `p0 = PACK(...)` write-back, `(a) | ((a) & 0)`
 *   lifetime no-ops (rule 50): `mask++; mask--;`, `under++; under--;`,
 *     `src++; src--;`
 *   eight separate value variables (q0..q3 for the second group) -> 97/238,
 *     much worse
 *   copying the final store into both branches (hoping for a cross-jump)
 *     -> 476 bytes
 *   a local copy for `out`, `out[0] = ...; out++;` -> same or worse
 *   `if (x < 0) ... x++;` for `x` (moving the increment to the end) -> same
 *   moving the `mask += 8; under += 8;` pair on the skipped path to the start or
 *     middle: the byte count drops to 470, but the instruction ORDER diverges
 *     from the ROM (218-219/235); in the ROM their place is AFTER the eight
 *     reads, as below.
 *   collapsing the pointer increments into one place AFTER the if/else: agbcc
 *     merges them and the output shrinks by 8 instructions (117/235).  Because
 *     the ROM duplicates them, the source must also write them separately in
 *     BOTH branches.
 *
 * STRUCTURAL DETAILS ALREADY RESOLVED (do not change these):
 *   - The outer loop must be a DOWN counter (`for (i = ROWS; i != 0; i--)`) and
 *     the row number must be written as `y0 + (ROWS - i)`.  An increasing `for`
 *     with `y = y0 + i` does not produce the ROM's `ldr y0 / adds #48 /
 *     subs counter` triple; parenthesization matters too: `y0 + ROWS - i` and
 *     `y0 - i + ROWS` produce a different instruction order, and only
 *     `y0 + (ROWS - i)` (and its equivalent `y0 - (i - ROWS)`) reproduces the
 *     ROM's.
 *   - The column test takes the form `if (x++ < 0)`: the ROM has
 *     `adds r0,r6,#0 / adds r6,#1 / cmp r0,#0`, i.e. increment FIRST.
 *   - The row test is a single expression with `||` (rule 60):
 *     `if (y >= height || y < 0)`.  The skipped body must come FIRST.
 *   - Group 1's pack+store must be written separately in BOTH branches; only
 *     group 2's is a shared tail (agbcc merges that one itself via cross-jump).
 *   - The `mask[0] & under[0]` order: agbcc reverses it and reads `under` first,
 *     and the ROM also reads arg5 first.  The increment orders were measured
 *     separately per branch (src/mask/under on the negative branch,
 *     under/mask/src on the blend branch).
 */

#define ROWS       48
#define ROW_STRIDE 48
#define GROUP      8

/* One pixel: source only if the column is negative, otherwise the blend.  Both
   branches advance the three pointers themselves (the ROM duplicated both). */
#define FETCH(v)                                    \
    if (x++ < 0) {                                  \
        v = src[0];                                 \
        src++;                                      \
        mask++;                                     \
        under++;                                    \
    } else {                                        \
        v = (mask[0] & under[0]) | src[0];          \
        under++;                                    \
        mask++;                                     \
        src++;                                      \
    }

#define PACK(a, b, c, d)  ((a) | ((b) << 4) | ((c) << 8) | ((d) << 12))

/* 0x08031844 */
void BlitStripClipLeft4bpp(s32 y0, s32 x0, s32 height, s32 stride,
                  const u8 *mask, const u8 *under, u16 *out, const u8 *src)
{
    s32 i;
    s32 x;
    s32 y;
    u32 p0;
    u32 p1;
    u32 p2;
    u32 p3;

    for (i = ROWS; i != 0; i--) {
        x = x0;
        y = y0 + (ROWS - i);
        if (y >= height || y < 0) {
            p0 = *src++;
            p1 = *src++;
            p2 = *src++;
            p3 = *src++;
            *out++ = PACK(p0, p1, p2, p3);
            p0 = *src++;
            p1 = *src++;
            p2 = *src++;
            p3 = *src++;
            mask += GROUP;
            under += GROUP;
        } else {
            FETCH(p0)
            FETCH(p1)
            FETCH(p2)
            FETCH(p3)
            *out++ = PACK(p0, p1, p2, p3);
            FETCH(p0)
            FETCH(p1)
            FETCH(p2)
            FETCH(p3)
        }
        *out++ = PACK(p0, p1, p2, p3);
        mask += stride;
        under += ROW_STRIDE - GROUP;
        src += ROW_STRIDE - GROUP;
    }
}
```

## 0x0803173E — WRONG BOUNDARY, NOT A FUNCTION

The entry recorded at 0x0803173E is a boundary error rather than a function.
This was measured, and no C was written for it.

Evidence:
1. The first instruction at 0x0803173E is `adds r4,#1`; there is no prologue.
2. The `b.n 0x80316CE` at 0x080317DC branches BACKWARD, to BEFORE the recorded
start -- so the body begins before 0x0803173E.
3. The epilogue at 0x080317DE is `pop {r3,r4,r5} / mov r8..sl / pop {r4-r7} /
pop {r0} / bx r0`; the matching prologue is at 0x080316B0:
`push {r4,r5,r6,r7,lr} / mov r7,sl / mov r6,r9 / mov r5,r8 /
push {r5,r6,r7} / sub sp,#4`.
4. The range 0x08031684-0x080316AF is a SEPARATE, complete function
(`push {r4,lr}` ... `bx r0`, with the pool word 0x02025810 at 0x080316AC) --
and it is not recorded in data/functions.csv at all.
Conclusion: the 186-byte "gap" is really two functions; the true boundary is
0x080316B0-0x080317EE (318 bytes), and 0x0803173E is the middle of its body.
No C was written for 0x0803173E.  (The real function is the sibling of
0x08031A1C: the same eight-nibble masked strip blitter, differing only in row
count and parameter layout.)
