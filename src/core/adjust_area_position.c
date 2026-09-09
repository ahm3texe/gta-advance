/* Corrects the fixed-point position with the transition offset when the area depth changes. */

#include "gba_types.h"

#define AREA_VERTICAL_STEP 0x07000000
#define AREA_LARGE_STEP    0x0C800000

typedef struct PositionPair {
    s32 x;
    s32 y;
} PositionPair;

typedef struct RecordBlock {
    s32 index;
    u8 pad04[12];
} RecordBlock;

typedef struct SessionDepth {
    u8 pad00[0x10];
    u8 depth;
} SessionDepth;

extern RecordBlock gRecordIndex;
extern SessionDepth gUnk02010C60;
extern u8 gLoopState;

/* 0x08051540 */
void AdjustAreaPosition(PositionPair *position)
{
    s32 current;
    u32 depth;

    current = gRecordIndex.index;
    depth = gUnk02010C60.depth;
    if (gLoopState == 4 && depth != 0 && --depth != current) {
        if (current == 0 && depth == 1) {
            position->x = 0x1FC10000;
            position->y -= AREA_VERTICAL_STEP;
        }
        if (current == 1) {
            if (depth == 0) {
                position->x = 0x003F0000;
                position->y += AREA_VERTICAL_STEP;
            }
            if (depth == 2) {
                position->x = 0x1FC10000;
                position->y += AREA_LARGE_STEP;
            }
        }
        if (current == 2 && depth == 1) {
            position->x = 0x003F0000;
            position->y -= AREA_LARGE_STEP;
        }
    }
}
