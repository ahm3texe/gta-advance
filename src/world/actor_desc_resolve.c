/* Resolve an actor descriptor — 0x0801952C-0x0801958F
 *
 * gRam03000078 holds the local player's slot (0 or 1). If that slot in the
 * actor's +0x20 array is 0xFDFD (empty), use the other slot; only one entry is
 * populated outside two-player mode.
 *
 * Resolve the selected ID to a ROM node through the gRom08BD3448 root and store
 * the node at actor +0x24. Shift the node's first byte by 16 and store it at
 * +0x0C; use the signed phase at +0x12 to select the final child node and return
 * its +0x14 field.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_desc_resolve.c
 */

#include "gba_types.h"

#define SLOT_EMPTY  0xFDFD

/* ROM root at 0x08BD3448; same view as entries_b1.c. */
typedef struct RomNode {
    u32             pad00;
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;
extern u32 gRam03000078;

typedef struct Actor {
    u8       pad00[4];
    u16      kind;              /* +0x04 */
    u8       pad06[6];
    u32      glyph;             /* +0x0C */
    u8       pad10[2];
    s16      phase;             /* +0x12 */
    u8       pad14[12];
    u16      ids[2];            /* +0x20 */
    RomNode *node;              /* +0x24 */
} Actor;

/* 0x0801952C */
u32 ResolveActorDesc(Actor *t)
{
    u32 sel;
    u32 idx;
    RomNode *node;
    RomNode *sub;

    sel = gRam03000078;
    if (t->ids[sel] == SLOT_EMPTY) {
        if (sel == 0)
            idx = 1;
        else
            idx = 0;
    } else {
        idx = sel;
    }

    node = gRom08BD3448.slots[t->ids[idx]];
    t->node = node;
    sub = node->slots[t->kind];
    t->glyph = ((u8 *)sub)[0] << 16;
    return sub->slots[t->phase]->unk14;
}
