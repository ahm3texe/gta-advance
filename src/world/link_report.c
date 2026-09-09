/* Populate the link summary record — 0x08066AA8-0x08066B3F
 *
 * Seven query calls each store both their return value and one-word stack
 * output in the record. Copy a 36-byte block from save buffer +0x64 (agbcc
 * emits three three-register ldmia/stmia pairs), then fill the final field
 * with a call to 0x08067014.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/link_report.c
 */

#include "gba_types.h"

#define QUERY_A       0x5E
#define QUERY_B       0x4F
#define GROUP_FIVE    5
#define GROUP_SEVEN   7

typedef struct StatsBlock {
    u32 words[9];               /* 36 bytes */
} StatsBlock;

typedef struct LinkReport {
    u16        head;            /* +0x00 */
    u16        headAlt;         /* +0x02 */
    u8         queryA;          /* +0x04 */
    u8         queryAAlt;       /* +0x05 */
    u8         queryB;          /* +0x06 */
    u8         queryBAlt;       /* +0x07 */
    u8         g5v1;            /* +0x08 */
    u8         g5v1Alt;         /* +0x09 */
    u8         g5v2;            /* +0x0A */
    u8         g5v2Alt;         /* +0x0B */
    u8         g5v3;            /* +0x0C */
    u8         g5v3Alt;         /* +0x0D */
    u8         summary;         /* +0x0E */
    u8         g7v2;            /* +0x0F */
    u8         g7v2Alt;         /* +0x10 */
    u8         pad11[3];
    StatsBlock stats;           /* +0x14 */
} LinkReport;

typedef struct SaveStats {
    u8         pad00[0x64];
    StatsBlock stats;           /* +0x64 */
} SaveStats;

extern SaveStats gSaveBuffer;

extern s32 FUN_080558e4(s32 *out);
extern s32 FUN_08030390(u32 query, s32 *out);
extern s32 FUN_080563c0(u32 kind, u32 variant, s32 *totalOut);
extern s32 FUN_08067014(LinkReport *report);

/* 0x08066AA8 */
void BuildLinkReport(LinkReport *report)
{
    s32 extra;

    report->head      = FUN_080558e4(&extra);
    report->headAlt   = extra;

    report->queryA    = FUN_08030390(QUERY_A, &extra);
    report->queryAAlt = extra;

    report->queryB    = FUN_08030390(QUERY_B, &extra);
    report->queryBAlt = extra;

    report->g5v1      = FUN_080563c0(GROUP_FIVE, 1, &extra);
    report->g5v1Alt   = extra;

    report->g5v2      = FUN_080563c0(GROUP_FIVE, 2, &extra);
    report->g5v2Alt   = extra;

    report->g5v3      = FUN_080563c0(GROUP_FIVE, 3, &extra);
    report->g5v3Alt   = extra;

    report->g7v2      = FUN_080563c0(GROUP_SEVEN, 2, &extra);
    report->g7v2Alt   = extra;

    report->stats     = gSaveBuffer.stats;

    report->summary   = FUN_08067014(report);
}
