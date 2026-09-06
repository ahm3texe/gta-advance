#ifndef GUARD_COMM_BLOCK_H
#define GUARD_COMM_BLOCK_H

#include "gba_types.h"

/* Baglanti (SIO) blogu — gRam02036338'in gosterdigi yapi.
 *
 * Dort kaynak bu blogu kullaniyor ve her biri farkli alanlarini goruyor:
 *   src/world/comm_flag.c      byte0, ready06
 *   src/world/link_hw_reset.c  ready06
 *   src/world/link_init.c      +0x14..0x2C kurulum alanlari ve tamponlar
 *   src/world/link_packet.c    baslik alanlari ve paket
 * Ayni sembol icin farkli struct govdeleri tanimlamak tutarlilik
 * denetimini hakli olarak durduruyor; tek tanim burada.
 *
 * Blok toplam 464 bayt: kurulum CpuSet ile tam bu kadarini sifirliyor.
 */

typedef struct LinkPacket {
    u8  ident;                          /* +0x00 */
    u8  mix;                            /* +0x01 */
    u16 checksum;                       /* +0x02 */
    u8  payload[16];                    /* +0x04 */
} LinkPacket;

typedef struct CommBlock {
    u8         byte0;                   /* +0x00 */
    u8         pad01;
    u8         byte02;                  /* +0x02 */
    u8         byte03;                  /* +0x03 */
    u8         ready;                   /* +0x04 */
    u8         pad05;
    u8         ready06;                 /* +0x06 */
    u8         pad07[4];
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
