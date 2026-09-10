/* The owner behind a record id — 0x0803101C-0x08031033
 *
 * Resolves the operand to an actor and answers its +0x28 owner, or 0 when
 * nothing resolves.
 *
 * Rule 73: the owner read is written AFTER the label, so it is the one that
 * lands first in the ROM.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/resolve_owner.c
 */

#include "gba_types.h"

typedef struct RecordOwner RecordOwner;

typedef struct RecordActor {
    u8           pad00[0x28];
    RecordOwner *owner;         /* +0x28 */
} RecordActor;

extern RecordActor *ResolveRecordNodeChain(u16 id);

/* 0x0803101C */
RecordOwner *FUN_0803101c(u16 id)
{
    RecordActor *actor = ResolveRecordNodeChain(id);

    if (actor != 0) goto have;
    return 0;
have:
    return actor->owner;
}
