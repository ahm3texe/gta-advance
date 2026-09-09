/* A limit from the save buffer, or from the status record — 0x080321BC-0x080321EB
 *
 * The same shape as the two getters in read_counter_pair.c, with one extra
 * condition: kind 2 is only honoured while byte 12 of gGameState is clear, the
 * byte data/ram_map.csv records as selecting the VBlank frame-delay behaviour.
 * Kind 1 does not consult it at all.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/status/read_limit_or_status.c
 */

#include "gba_types.h"
#include "../status/status_types.h"

#define KIND_SAVE    1
#define KIND_STATUS  2
#define TWO_PLAYER   12         /* gGameState byte 12 */

extern MenuCtx      gSaveBuffer;
extern StatusRecord gRam02026CD0;
extern u8           gGameState[];

/* 0x080321BC */
u32 FUN_080321bc(u32 kind)
{
    if (kind == KIND_SAVE)
        return gSaveBuffer.w32;
    if (gGameState[TWO_PLAYER] != 0) goto zero;
    if (kind == KIND_STATUS) goto status;
zero:
    return 0;
status:
    return gRam02026CD0.h26;
}
