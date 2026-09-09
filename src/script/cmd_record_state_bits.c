/* Script commands: change bits in the record's +0x1E state
 * 0x0805AE2C-0x0805AE4D, 0x0805AE50-0x0805AE75, 0x0805AE78-0x0805AE9D
 *
 * Three adjacent handlers over the same halfword: set bit 1, set bit 9, clear
 * bit 9. Each answers 0 when the operand resolves to nothing.
 *
 * Rule 71: the zero answer is at the END in all three, so the body is the
 * `then` arm of a two-armed if.
 *
 * The three constants show the three ways agbcc reaches a 16-bit value: 2 as a
 * bare `movs`, 0x200 as `movs #128 / lsls #2`, and ~0x200 through the literal
 * pool, since neither form can build 0xFDFF.
 *
 * Rule 33 does NOT apply to the first and third. The ROM materialises their
 * constants first and applies the field to them in place, which is what a plain
 * `|=` and `&=` on the field already produce here; a separate constant local is
 * not needed and does not change the output. The middle one was measured with
 * both spellings for the same reason and the plain `|=` is what reproduces the
 * ROM's extra `adds r0,r2,#0`, which comes from the register allocator rather
 * than from anything the source says.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_record_state_bits.c
 */

#include "gba_types.h"

#define STATE_BIT1  2
#define STATE_BIT9  (128 << 2)

typedef struct StateNode {
    u8  pad00[0x1E];
    u16 state;                  /* +0x1E */
} StateNode;

extern StateNode *GetOrCreateRecordNode(u16 id);

/* 0x0805AE2C */
u32 FUN_0805ae2c(u32 a, u32 id)
{
    StateNode *node = GetOrCreateRecordNode(id);
    u32 result;

    if (node != 0) {
        node->state |= STATE_BIT1;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

/* 0x0805AE50 */
u32 FUN_0805ae50(u32 a, u32 id)
{
    StateNode *node = GetOrCreateRecordNode(id);
    u32 result;

    if (node != 0) {
        node->state |= STATE_BIT9;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

/* 0x0805AE78 */
u32 FUN_0805ae78(u32 a, u32 id)
{
    StateNode *node = GetOrCreateRecordNode(id);
    u32 result;

    if (node != 0) {
        node->state &= ~STATE_BIT9;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}
