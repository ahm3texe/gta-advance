/* Dort duzleme gore noktanin negatif/sinir taraflarini bitlerle siniflar.
 * PARK: yardimci fonksiyonlu ve acik dort bloklu bicimler denendi. En iyi
 * dogal C 166/174 bayt; kalan fark yerel/yazmac yasam araligi dagitimi. */

#include "gba_types.h"

/* 0x0800AF48 */
u32 FUN_0800af48(const s32 *point, const s32 *planes)
{
    u32 side;
    s32 distance;
    s32 y;
    s32 z;
    s32 w;
    s32 x;
    u32 result;

    distance = *planes * *point + point[1] * planes[1]
             + point[2] * planes[2] + point[3];
    if (distance < 1) {
        result = 2;
        if (distance < 0)
            result = 1;
    } else {
        result = 0;
    }

    x = *point;
    y = point[1];
    z = point[2];
    w = point[3];

    distance = planes[5] * x + y * planes[6] + z * planes[7] + w;
    if (distance < 1) {
        if (distance < 0)
            side = 1;
        else
            side = 2;
        result |= side;
    }

    distance = planes[10] * x + y * planes[11] + z * planes[12] + w;
    if (distance < 1) {
        if (distance < 0)
            side = 1;
        else
            side = 2;
        result |= side;
    }

    distance = planes[15] * x + y * planes[16] + z * planes[17] + w;
    if (distance < 1) {
        if (distance < 0)
            side = 1;
        else
            side = 2;
        result |= side;
    }

    return result;
}
