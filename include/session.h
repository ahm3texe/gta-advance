#ifndef GUARD_SESSION_H
#define GUARD_SESSION_H

#include "gba_types.h"

/* Oturum blogu — gRam02026DF0 (0x02026DF0).
 *
 * Iki kaynak kullaniyor ve ayni sembol icin farkli struct govdeleri
 * tutarlilik denetimini durduruyor; tek tanim burada.
 *   src/world/area_cleanup_b1.c   tum blogu goruyor
 *   src/world/band_a_30f78.c      yalnizca +0x00 etkinlik bayragini siliyor
 */
typedef struct Snapshot {
    u32 word[9];
} Snapshot;

typedef struct Record {
    u8 pad00[0x2C];
} Record;

typedef struct Session {
    u8       unk00;             /* +0x00 — etkinlik bayragi */
    u8       pad01[3];
    u32      unk04;             /* +0x04 */
    Record   unk08;             /* +0x08 */
    Record   unk34;             /* +0x34 */
    Snapshot unk60;             /* +0x60 */
} Session;

extern Session gRam02026DF0;

#endif /* GUARD_SESSION_H */
