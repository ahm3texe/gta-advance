/* Unmasked/masked strip blitter and slot sub-object release
 * — 0x08031684-0x080317ED
 *
 * BOUNDARY NOTE: data/functions.csv held a single 0x0803173E record here for
 * a long time. That address is not a function; the apparent
 * `bl 0x0803173E` at 0x0803073C is really a pool word (0xF000FFFF), and
 * discover_functions.py mistook the pool for code. The measured real layout
 * is 0x08031684 (44 bytes) and 0x080316B0 (318 bytes). Because the two are
 * ADJACENT, they can live in the same translation unit.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/video/blit_strip_plain.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

/* ---- 0x08031684 — releases the slot's sub-object ---------------------
 *
 * The 24 x 180-byte slot array at gRam02025810 (see
 * docs/GRAM02025810_LAYOUT.md and src/world/release_slot.c). If the +0xB1
 * flag inside the element is set, the sub-object at +0x64 inside the element
 * is released and the flag is cleared.
 *
 * The two bases must be built in SEPARATE LOCALS (the same measurement as in
 * release_slot.c): the flag adds up as (base + scale) + 0xED while the
 * sub-object adds up as scale + (base + 0xA0) -- these are the ROM's two
 * different association orders.
 * ------------------------------------------------------------------ */

#define SLOT_STRIDE  180            /* 0xB4 */
#define OFF_SUB      0xA0           /* block start; +0x64 inside the element */
#define OFF_FLAG     0xED           /* block start; +0xB1 inside the element */

extern void ReleaseObject(u8 *sub);

/* 0x08031684 */
void ReleaseSlotSub(u32 index)
{
    u8 *base;
    u8 *flag;
    u8 *subBase;
    u32 scaled;

    base = gRam02025810;
    scaled = index * SLOT_STRIDE;
    flag = base + scaled + OFF_FLAG;
    if (*flag == 0)
        return;

    subBase = base + OFF_SUB;
    ReleaseObject(subBase + scaled);
    *flag = 0;
}

/* ---- 0x080316B0 — 318 bytes, 8x48 4bpp strip blitter -----------------
 *
 * Same family as 0x08031A1C in src/video/blit_strip_4bpp.c; the difference
 * is that THERE IS NO COLUMN CLIPPING HERE. If the row falls outside the
 * vertical range (row >= rowLimit or row < 0), the eight source nibbles are
 * taken as they are; inside the range all of them are blended with
 * `(*mask & *under) | *src`. The eight values are packed into four-bit fields
 * and written to dst as two halfwords.
 *
 * Three measurements inherited from the sibling that ALSO HOLD HERE:
 *  1. The loop must be written DESCENDING and the row RECOMPUTED as
 *     `rowBase + (48 - i)`; the ROM emits `movs #48 / add ip,-1 / cmp #0 /
 *     beq`.
 *  2. The clip test must be a SINGLE `if (a || b)`; agbcc emits
 *     `bge FAST / bge MASKED / (fallthrough) FAST`, which is the ROM's block
 *     order.
 *  3. On the FAST path, mask/under must be incremented PER NIBBLE. The ROM
 *     shows a single `adds r6,#8 / adds r5,#8`, but writing it that way in
 *     the source shifts the temporaries' lifetimes; the compiler performs the
 *     merge itself, AFTER allocation.
 * End-of-row strides: mask is variable (maskStep), under and src are 40 bytes
 * (the row stride is 48 because the eight nibbles already advance one by one).
 * ------------------------------------------------------------------ */

#define STRIP_ROWS   48
#define ROW_BYTES    40

/* 0x080316B0 */
void BlitStripPlain4bpp(s32 rowBase, s32 rowLimit, s32 maskStep,
                        const u8 *mask, const u8 *under, u16 *dst,
                        const u8 *src)
{
    s32 i;
    s32 row;
    u32 n0;
    u32 n1;
    u32 n2;
    u32 n3;

    for (i = STRIP_ROWS; i != 0; i--) {
        row = rowBase + (STRIP_ROWS - i);
        if (row >= rowLimit || row < 0) {
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            n0 = *src++;
            mask++;
            under++;
            n1 = *src++;
            mask++;
            under++;
            n2 = *src++;
            mask++;
            under++;
            n3 = *src++;
            mask++;
            under++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        } else {
            n0 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n1 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n2 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n3 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
            n0 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n1 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n2 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            n3 = (*mask & *under) | *src;
            under++;
            mask++;
            src++;
            *dst++ = n0 | (n1 << 4) | (n2 << 8) | (n3 << 12);
        }
        mask += maskStep;
        under += ROW_BYTES;
        src += ROW_BYTES;
    }
}
