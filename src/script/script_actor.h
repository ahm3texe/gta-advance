#ifndef GUARD_SCRIPT_ACTOR_H
#define GUARD_SCRIPT_ACTOR_H

/* Shape of the actor and owner records the handler block at
 * 0x08059AE4-0x0805AB14 reaches through ResolveRecordNodeChain.
 *
 * Only the fields these handlers touch are named. The padding is not a claim
 * about the records' real extent; it only fixes the offsets that were measured.
 * Kept in one header because check_consistency requires every use of a shared
 * symbol to agree on its layout. */

#include "gba_types.h"

typedef struct ScriptOwner {
    u8  pad00[0x0C];
    u32 flags;                  /* +0x0C */
} ScriptOwner;

typedef struct ScriptActor {
    u8           pad00[0x28];
    ScriptOwner *owner;         /* +0x28 */
} ScriptActor;

#endif /* GUARD_SCRIPT_ACTOR_H */
