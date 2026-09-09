#ifndef GUARD_COMM_BLOCK_H
#define GUARD_COMM_BLOCK_H

#include "gba_types.h"

/* Serial link (SIO) state block pointed to by gRam02036338.
 *
 * Shared users access different parts of this block:
 *   src/world/comm_flag.c      byte0 and ready06
 *   src/world/link_hw_reset.c  ready06
 *   src/world/link_init.c      setup fields at +0x14..+0x2C and buffers
 *   src/world/link_packet.c    header fields and packet data
 * A single definition avoids conflicting struct layouts for the same symbol,
 * which are rejected by the consistency check.
 *
 * The block occupies 464 bytes; initialization clears exactly this many bytes
 * with CpuSet.
 */

typedef struct LinkPacket {
    u8  ident;                          /* +0x00 */
    u8  mix;                            /* +0x01 */
    u16 checksum;                       /* +0x02 */
    u8  payload[16];                    /* +0x04 */
} LinkPacket;

/* Eight-byte counter block at 0x02036330. Its size was established from
 * the Memset call at 0x08066144. Users include:
 *   src/world/link_session_reset.c  clears the field at +0x02
 *   src/world/sio_driver.c          writes the field at +0x00
 * This shared definition avoids conflicting extern types for the same symbol,
 * which are rejected by the consistency check. */
typedef struct LinkCounters {
    u16 half00;                         /* +0x00 */
    u16 half02;                         /* +0x02 */
    u8  pad04[4];
} LinkCounters;

extern LinkCounters gRam02036330;

typedef struct CommBlock {
    u8         byte0;                   /* +0x00 */
    u8         byte01;                  /* +0x01 */
    u8         byte02;                  /* +0x02 */
    u8         byte03;                  /* +0x03 */
    u8         ready;                   /* +0x04 */
    u8         arrived;                 /* +0x05 */
    u8         ready06;                 /* +0x06 */
    u8         errorBit;                /* +0x07 */
    u8         pad08;
    u8         retry;                   /* +0x09 */
    u8         pad0A;
    u8         ident;                   /* +0x0B */
    u8         pad0C[0x14 - 0x0C];
    s32        sendLen;                 /* +0x14 */
    s32        recvLen;                 /* +0x18 */
    LinkPacket *packetPtr;              /* +0x1C */
    void      *bufBPtr;                 /* +0x20 */
    void      *bufCPtr;                 /* +0x24 */
    void      *bufDPtr;                 /* +0x28 */
    void      *bufEPtr;                 /* +0x2C */
    LinkPacket packet;                  /* +0x30 */
    u8         pad44[0x48 - 0x44];
    u8         bufB[0x60 - 0x48];       /* +0x48 */
    u8         bufC[0xC0 - 0x60];       /* +0x60 */
    u8         bufD[0x120 - 0xC0];      /* +0xC0 */
    u8         bufE[0x1D0 - 0x120];     /* +0x120 */
} CommBlock;

extern CommBlock *gRam02036338;

#endif /* GUARD_COMM_BLOCK_H */
