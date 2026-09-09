/* The id match query — 0x08019620-0x0801966F
 *
 * Tests the entity at the holder's +0x1C against the given id through two of
 * its fields (u32 +0x28 and u16 +0x06).  If the id is greater than 0x3FFF it
 * is first turned into an alias via gRom08CA45CC[id - 0x4000] and that is
 * tried as well.
 *
 * THE ENTITY IS READ TWICE: on the second round the ROM reloads +0x1C
 * (ldr r1,[r4,#28]), so the source uses a SEPARATE local on the second round.
 * Held in a single local, the same register (r2) is reused.
 *
 * The early "return 0" exit must also be written as a nested
 * `if (e != 0) { ... }`; with a separate early return the block order is
 * reversed (the return 1 and return 0 blocks swap places).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/actor_id_matches.c
 */

#include "gba_types.h"

#define ID_DIRECT_MAX  0x3FFF
#define ID_BASE        0x4000

typedef struct Ent {
    u8  pad00[6];
    u16 tag;                    /* +0x06 */
    u8  pad08[0x20];
    u32 id;                     /* +0x28 */
} Ent;

typedef struct Holder {
    u8   pad00[0x1C];
    Ent *ent;                   /* +0x1C */
} Holder;

extern const u16 gRom08CA45CC[];

/* 0x08019620 */
u32 ActorIdMatches(Holder *h, u32 id)
{
    Ent *e;

    e = h->ent;
    if (e != 0) {
        if (id > ID_DIRECT_MAX) {
            u16 t;

            t = gRom08CA45CC[id - ID_BASE];
            if (e->id == t)
                return 1;
            if (e->tag == t)
                return 1;
        }
        {
            Ent *cur;

            cur = h->ent;
            if (cur->id == id)
                return 1;
            if (cur->tag == id)
                return 1;
        }
    }
    return 0;
}
