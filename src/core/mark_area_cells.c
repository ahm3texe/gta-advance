/* Marks the coordinate list attached to the object into the active area cells.
 * PARKED: the parameter width, for/while and the first counter's lifetime were
 * tried. The body is 164/164 with only 20 bytes differing. The load order of
 * the two global literals
 * (gRam020307F0 / gGameState) remains reversed in natural C. */

#include "gba_types.h"

typedef struct CellEntry {
    u16 x;
    u16 y;
    u8 pad04[4];
} CellEntry;

typedef struct CellSource {
    u8 pad00[0x3C];
    u16 count;
    u8 pad3E[2];
    CellEntry entries[1];
} CellSource;

typedef struct Owner {
    u8 pad00[0x24];
    CellSource *source;
} Owner;

typedef struct CellGrid {
    s32 originY;
    s32 originX;
    u8 cells[1];
} CellGrid;

extern CellGrid gRam020307F0;
extern u8 gGameState[16];

/* 0x080515D0 */
void FUN_080515d0(Owner *owner, s32 value)
{
    CellSource *source;
    u32 initialCount;
    u32 count;
    u16 *entry;

    source = owner->source;
    if (source != 0 && *((u8 *)source + 0x82) != 0) {
        initialCount = source->count;
        entry = (u16 *)source->entries;
        if (initialCount != 0) {
        count = initialCount;
        do {
            s32 row;
            u32 column;
            u8 *cell;

            column = entry[0] - gRam020307F0.originX;
            row = entry[1] - gRam020307F0.originY;
            if (column < 0x20 && row >= 0 && row < 0x20) {
                cell = &gRam020307F0.cells[row * 0x10 + column];
                if (*cell == 0)
                    *cell = value;
            }
            if (gGameState[12] != 0) {
                column = entry[0] - gRam020307F0.originX;
                row = entry[1] - gRam020307F0.originY;
                if (column < 0x20 && row >= 0 && row < 0x20) {
                    cell = &gRam020307F0.cells[row * 0x10 + column];
                    if (*cell == 0)
                        *cell = value;
                }
            }
            count--;
            entry += 4;
        } while (count != 0);
        }
    }
}
