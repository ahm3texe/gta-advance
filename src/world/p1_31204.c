/* Advance the text state and close it once empty — 0x08031204.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x08031204.json. */
#include "gba_types.h"
#include "phase1_types.h"
#include "ram_symbols.h"

extern void *FUN_0802e3fc(u32,void *,u32,void *);
extern void FUN_0802ead4(void);
void FUN_08031204(void)
{
    Phase1TextState *state = (Phase1TextState *)gRam02025810;
    state->previous = state->current;
    state->current = FUN_0802e3fc(39,state->current,1,state->buffer);
    if (!state->current) FUN_0802ead4();
}
