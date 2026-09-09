#ifndef GUARD_SESSION_H
#define GUARD_SESSION_H

#include "gba_types.h"

/* Session state block: gRam02026DF0 at 0x02026DF0.
 *
 * This shared definition avoids conflicting struct layouts for the same
 * symbol, which are rejected by the consistency check. Users include:
 *   src/world/area_cleanup_b1.c  accesses the full block
 *   src/world/band_a_30f78.c     only clears the activity flag at +0x00
 */
typedef struct Snapshot {
    u32 word[9];
} Snapshot;

typedef struct Record {
    u8 pad00[0x2C];
} Record;

typedef struct Session {
    u8       unk00;             /* +0x00: activity flag */
    u8       pad01[3];
    u32      unk04;             /* +0x04 */
    Record   unk08;             /* +0x08 */
    Record   unk34;             /* +0x34 */
    Snapshot unk60;             /* +0x60 */
} Session;

extern Session gRam02026DF0;

#endif /* GUARD_SESSION_H */
