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

/* 0x02036330'daki 8 baytlik sayac blogu. Boyut 0x08066144'teki
 * Memset cagrisindan OLCULDU. Iki kaynak kullaniyor:
 *   src/world/link_session_reset.c  +0x02'yi sifirliyor
 *   src/world/sio_driver.c          +0x00'i yaziyor
 * Ayni sembol icin farkli extern turleri tutarlilik denetimini
 * durduruyor; tek tanim burada. */
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
