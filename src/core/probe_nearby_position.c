/* Tests the position at four horizontal/vertical offsets; restores it if none works. */

#include "gba_types.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

extern s32 FUN_0802338c(Vec3 *position);

/* 0x08055970 */
void ProbeNearbyPosition(Vec3 *position)
{
    Vec3 saved;

    if (FUN_0802338c(position) == 0) {
        saved = *position;
        position->x -= 0x180000;
        if (FUN_0802338c(position) == 0) {
            *position = saved;
            position->x += 0x180000;
            if (FUN_0802338c(position) == 0) {
                *position = saved;
                position->y += 0x180000;
                if (FUN_0802338c(position) == 0) {
                    *position = saved;
                    position->y -= 0x180000;
                    if (FUN_0802338c(position) == 0)
                        *position = saved;
                }
            }
        }
    }
}
