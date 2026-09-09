/* Initialize the link block — 0x08066568-0x0806660B
 *
 * Clear 464 bytes (CpuSet, fixed source, 116 words), disable the serial
 * interrupt, set RCNT/SIOCNT to multiplayer mode, assign five buffer pointers
 * inside the block, then enable the serial interrupt on exit.
 *
 * IE writes occur with IME disabled; ordering was read from the ROM. agbcc
 * keeps constant 1 in a high register (r8) because it is used twice.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_init.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

#define CPUSET_FILL_WORDS  0x05000074   /* fixed source + 32-bit + 116 words */
#define IE_KEEP_MASK       0xFF3F
#define IE_SERIAL          0x0080
#define RCNT_SIO           0x0000
#define SIOCNT_MULTI       0x2000
#define SIOCNT_ENABLE      0x4003

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x08066568 */
void InitLinkBlock(CommBlock *block)
{
    s32 zero;

    gRam02036338 = block;

    zero = 0;
    CpuSet(&zero, block, CPUSET_FILL_WORDS);

    REG_IME = 0;
    REG_IE = REG_IE & IE_KEEP_MASK;
    REG_IME = 1;

    REG_RCNT   = RCNT_SIO;
    REG_SIOCNT = SIOCNT_MULTI;
    REG_SIOCNT = REG_SIOCNT | SIOCNT_ENABLE;

    gRam02036338->sendLen   = 12;
    gRam02036338->recvLen   = 12;
    gRam02036338->packetPtr = &gRam02036338->packet;
    gRam02036338->bufBPtr   = gRam02036338->bufB;
    gRam02036338->bufCPtr   = gRam02036338->bufC;
    gRam02036338->bufDPtr   = gRam02036338->bufD;
    gRam02036338->bufEPtr   = gRam02036338->bufE;

    REG_IME = 0;
    REG_IE = REG_IE | IE_SERIAL;
    REG_IME = 1;
}
