#ifndef GUARD_STATUS_TYPES_H
#define GUARD_STATUS_TYPES_H

/* The two records the status getters at 0x08031EEC-0x080321EB read from.
 *
 * MenuCtx is gSaveBuffer (0x02000D50) and its body is copied verbatim from
 * src/ui/init_menu_session.c, which must keep the same layout: the consistency
 * check requires one struct body per shared symbol. The +0x0C, +0x0E and +0x32
 * fields were carved out of that file's padding by these getters, without
 * moving anything that was already named.
 *
 * StatusRecord is gRam02026CD0. Its 80 bytes and its +0x02 and +0x4C fields are
 * what data/ram_map.csv already records; +0x00 and +0x26 are added here. */

#include "gba_types.h"

typedef struct Item { u8 pad00[20]; } Item;

typedef struct MenuCtx { u8 active; u8 pad01[11]; u16 w0C; u16 w0E; u32 w10; u32 w14;
                         u8 pad18[26]; u16 w32; u8 pad34[84];
                         u8 head[0x18]; u8 count; u8 pad; u8 pad2; u8 pad3; Item items[1]; } MenuCtx;

typedef struct StatusRecord {
    u16 h00;                    /* +0x00 */
    u16 h02;                    /* +0x02 */
    u8  pad04[0x22];
    u16 h26;                    /* +0x26 */
    u8  pad28[36];
    u32 w4C;                    /* +0x4C */
} StatusRecord;                 /* 80 bytes, per data/ram_map.csv */

#endif /* GUARD_STATUS_TYPES_H */
