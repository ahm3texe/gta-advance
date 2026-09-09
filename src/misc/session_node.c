/* Session node reset — 0x0803C798-0x0803C7B9
 *
 * MATCHES (byte-matching, 34 bytes).
 *
 * The solution: the flag field is `s8`, not `u8`. The ROM builds the mask as
 * 32 bits (`movs r0,#5 / negs r0,r0` = -5) and does not narrow it to a byte
 * (it is not `movs r0,#251`). The reason: agbcc/gcc only narrows the constant
 * of an `and` when it KNOWS the upper bits of the ANDed value are zero. With a
 * `u8` field the load counts as a zero-extension, so nonzero_bits = 0xFF and
 * -5 drops to 0xFB. With a signed field the upper bits are unknown and the
 * mask stays 32 bits; because the value is written back with `strb`, the load
 * still stays `ldrb`.
 *
 * An equivalent form giving the same output: making the field a bitfield
 * (`u8 f0:2; u8 f2:1; u8 f3:5;` + `gSessionPtr->f2 = 0;`) -- bitfield
 * insertion/extraction also builds the mask in word mode. Since the meaning of
 * the other bits is unknown, the single signed field was preferred.
 *
 * Tried and REJECTED (all produced `movs r0,#251`): the masks ~4, -5 and
 * 0xFFFFFFFB on a `u8` field; the cast `(u8)((s32)flags & ~4)`; reading
 * through a u32 local. A `volatile u8` field not only narrows the mask but
 * also adds an extra `ldrb`. An `s32` local mask produces the right constant
 * but inserts an `add r0, r2, #0` copy in between.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/session_node.c
 */

#include "gba_types.h"

typedef struct {
    u8  unk00[40];
    u32 unk28;          /* 0x28 */
    u32 unk2C;          /* 0x2C */
} Node;

typedef struct {
    u8    unk00[28];
    Node *node;         /* 0x1C */
} Context;

typedef struct {
    Context *context;   /* 0x00 */
    u8       unk04[4];
    s8       flags;     /* 0x08 */
} Session;

#define SESSION_FLAG_BIT  4
#define NODE_RESET_VALUE  24

extern Session *gSessionPtr;

/* 0x0803C798 */
void ResetSessionNode(void)
{
    Node *node;

    node = gSessionPtr->context->node;
    if (node != 0) {
        node->unk28 = NODE_RESET_VALUE;
        node->unk2C = 0;
    }

    gSessionPtr->flags &= ~SESSION_FLAG_BIT;
}
