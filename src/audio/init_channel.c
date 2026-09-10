/* Initialise a channel record — 0x08034F9C-0x08034FAD
 *
 * The +0x00 word takes the argument and six other fields are zeroed. The ROM
 * materialises the zero FIRST and stores it to +0x04 before the argument goes
 * to +0x00, so the byte at +0x04 is cleared ahead of the word.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/audio/init_channel.c
 */

#include "gba_types.h"

typedef struct Channel {
    u32 source;                 /* +0x00 */
    u8  state;                  /* +0x04 */
    u8  flag;                   /* +0x05 */
    u8  pad06[2];
    u32 unk08;                  /* +0x08 */
    u32 pad0C;
    u32 unk10;                  /* +0x10 */
    u32 unk14;                  /* +0x14 */
    u32 unk18;                  /* +0x18 */
} Channel;

/* 0x08034F9C */
void FUN_08034f9c(Channel *channel, u32 source)
{
    channel->state = 0;
    channel->source = source;
    channel->flag = 0;
    channel->unk10 = 0;
    channel->unk08 = 0;
    channel->unk14 = 0;
    channel->unk18 = 0;
}
