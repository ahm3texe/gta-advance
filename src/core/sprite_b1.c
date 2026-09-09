/* FUN_08012cd8 @ 0x08012CD8, 320 bytes -- an RLE (run-length) decoder.
 *
 * DIAGNOSIS -- THE FILE NAME IS MISLEADING: "sprite_b1" came from a sibling
 * assignment; the body has NOTHING TO DO with sprites. The src[0] & 0xF0 ==
 * 0x30 check is the GBA compression header (0x10 LZ77, 0x20 Huffman, 0x30 RL).
 * The decompressed size is read as 24-bit little-endian from src[1..3], and
 * the data starts at src[4].
 *
 * The output is written 16-bit aligned -- byte stores are forbidden because
 * the destination is VRAM. So a byte landing on an odd address is carried into
 * the next halfword store as a "carry", and a leftover odd byte at the end of
 * a run carries into the next run. carry is a u8: every assignment produces
 * an lsls#24/lsrs#24.
 *
 * Bit 7 of the flag byte selects the run:
 *   set   -> length (flag & 0x7F) + 3, a single data byte is repeated
 *   clear -> length (flag & 0x7F) + 1, that many bytes are copied verbatim
 * In the copy arm the parity of the SOURCE address branches as well: if even,
 * a halfword is read directly; if odd, it is assembled by hand from two bytes.
 * The ROM repeats the ni/nj computation INSIDE both arms as well -- that was
 * left as it is.
 *
 * STATUS: the size MATCHES at 320/320 bytes, with 290/320 bytes IDENTICAL. The
 * instruction sequence (body, branches, loop forms, operand order) is
 * BYTE-FOR-BYTE the ROM's. The remaining 30 bytes are A SINGLE REGISTER SWAP:
 * the ROM puts `src` on the stack and gives sl to the copy arm's `tail`
 * temporary; agbcc does the opposite for us (src -> sl, tail -> sp+8). Both
 * have the same instruction count and the same size.
 *
 * THE MEASURED REASON (agbcc -dg dump; the global.c priority is
 * floor_log2(refs) * refs / live_length):
 *     src  : refs 11, live_length 122  -> 0.2705   (gets a register)
 *     tail : refs  6, live_length  46  -> 0.2609   (falls to the stack)
 * The order favors src by 3.7%. To flip it, src needs live_length >= 127, or
 * refs <= 10, or tail needs live_length <= 45.
 *
 * TRIED / ELIMINATED -- DO NOT RETRY; ADD anything new you try here:
 *  1. The scope of ni/nj/tail/n/s/d/b (inside the branch / the loop body / the
 *     function): 512 combinations swept. The ONLY winner: moving `ni` into the
 *     loop body (67 -> 30 bytes). Everything else is >= 30.
 *  2. Local declaration ORDER (all 6! permutations) -- agbcc is unaffected.
 *  3. `nj` at loop or function scope -- 308 bytes, the size breaks.
 *  4. Dropping `ni` or `nj` and writing `i += len` / `j++` -- 304..306 bytes.
 *  5. Swapping the ni/nj order (in the compressed arm) -- 322 bytes.
 *  6. `carry = (n != 0) ? ... : carry` as a ternary instead of `carry = tail`
 *     -- it loses the phi structure, 296 bytes.
 *  7. `carry` as u32 with `& 0xFF` -- 78 bytes of difference (ands instead of
 *     lsls/lsrs).
 *  8. Computing `len` inside the branches -- 330 bytes (agbcc does not hoist,
 *     and a separate `ands` comes out in both arms).
 *  9. Forcing src onto the stack with `u8 *volatile src` -- 288 bytes; it
 *     reloads inside every expression.
 * 10. Splitting the address computation (`s = src; s += j;`) ADDS an
 *     instruction to the pre-reload RTL without changing the output: src's
 *     live_length goes 122 -> 124. That does not reach the required 127; the
 *     split in the compressed arm contributes +0 (it falls AFTER src's last
 *     use).
 * 11. Splitting the header reads (`t = src[1]; size |= t;`) -- combine merges
 *     them back and live_length does not change.
 * 12. Using `tail` in the odd-address store to add a reference -- copy
 *     propagation folds it back into carry and the numbers do not change.
 * 13. Splitting the allocno with a byte copy (`b2 = b;`) -- the CLAUDE.md rule:
 *     it is always eliminated, and it was eliminated here too (live_length
 *     stayed at 122).
 * 14. decomp-permuter: after the 28th iteration every mutation gives a compile
 *     error; it could not be used on this body.
 *
 * A NOTE FOR THE NEXT STEP: the remaining difference is not in the SOURCE
 * STRUCTURE but in a single priority threshold. What to look for is a rewrite
 * that, without changing the output instructions, adds 3 more instructions to
 * the copy arm's PREFIX (BEFORE the `s = src` line) or removes 1 instruction
 * from BETWEEN `tail = carry` and `carry = tail`.
 *
 * Verification: make c-match FILE=src/core/sprite_b1.c
 */

#include "gba_types.h"

/* 0x08012CD8 */
void FUN_08012cd8(u8 *src, u8 *dst)
{
    s32 size;
    s32 i;
    s32 j;
    u8 carry;
    u32 flag;
    s32 len;

    /* Only an RL header (0x3n) is accepted. */
    if ((src[0] & 0xF0) != 0x30)
        return;

    size = src[2] << 8;
    size |= src[1];
    size |= src[3] << 16;

    carry = 0;
    j = 4;
    i = 0;
    while (i < size) {
        s32 ni;
        flag = src[j];
        len = flag & 0x7F;
        if ((flag & 0x80) != 0) {
            u8 *s;
            u8 *d;
            u32 b;
            s32 n;
            u32 tail;
            s32 nj;

            /* Repeat run: a single byte is written (len+3) times. */
            len += 3;
            j++;
            s = &src[j];
            d = &dst[i];
            b = *s;
            n = len;
            tail = carry;
            if (((u32)d & 1) != 0) {
                /* Odd address: take the previous byte along and store a halfword. */
                *(u16 *)(d - 1) = carry | (b << 8);
                d++;
                n--;
            }
            nj = j + 1;
            ni = i + len;
            while (n > 1) {
                *(u16 *)d = (b << 8) | b;
                d += 2;
                n -= 2;
            }
            if (n != 0)
                tail = b;
            carry = tail;
            j = nj;
            i = ni;
        } else {
            u8 *s;
            u8 *d;
            s32 n;
            u32 tail;
            s32 nj;

            /* Plain copy run: (len+1) bytes are transferred verbatim. */
            len += 1;
            j++;
            d = &dst[i];
            s = &src[j];
            n = len;
            tail = carry;
            if (((u32)d & 1) != 0) {
                *(u16 *)(d - 1) = carry | (*s << 8);
                d++;
                n--;
                s++;
            }
            if (((u32)s & 1) != 0) {
                /* The source is at an odd address: a halfword cannot be read, assemble it by hand. */
                ni = i + len;
                nj = j + len;
                while (n > 1) {
                    *(u16 *)d = (s[1] << 8) | s[0];
                    d += 2;
                    n -= 2;
                    s += 2;
                }
            } else {
                ni = i + len;
                nj = j + len;
                while (n > 1) {
                    *(u16 *)d = *(u16 *)s;
                    d += 2;
                    n -= 2;
                    s += 2;
                }
            }
            if (n != 0)
                tail = *s;
            carry = tail;
            j = nj;
            i = ni;
        }
    }
}
