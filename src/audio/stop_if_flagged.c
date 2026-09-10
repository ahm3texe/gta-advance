/* Stop the channel when its +0x05 flag is set — 0x08034FB4-0x08034FC7
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/stop_if_flagged.c
 */

#include "gba_types.h"

typedef struct Channel {
    u8 pad00[5];
    u8 flag;                    /* +0x05 */
} Channel;

extern void FUN_08034ad8(Channel *channel);

/* 0x08034FB4 */
void FUN_08034fb4(Channel *channel)
{
    if (channel->flag != 0)
        FUN_08034ad8(channel);
}
