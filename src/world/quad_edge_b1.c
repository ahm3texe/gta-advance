/* calisma kopyasi */
#include "gba_types.h"

s32 FUN_0800bdf8(const s32 poly[][3], u16 count, const s32 *point, s32 tolerance)
{
    s32 maxZ;
    s32 minZ;
    s32 i;
    s32 side;
    s32 j;

    maxZ = 0;
    minZ = 0;
    tolerance <<= 7;
    for (i = 0; i < count - 1; i++) {
        if (poly[i][2] > poly[i + 1][2]) {
            maxZ = poly[i][2];
            minZ = poly[i + 1][2];
        } else {
            maxZ = poly[i + 1][2];
            minZ = poly[i][2];
        }
    }
    if (maxZ < point[2] - tolerance) return 0;
    if (minZ > point[2] + tolerance) return 0;
    for (i = 0; i < count; i++) {
        /* `(i + 1) % count` YAZILAMAZ: agbcc bunu __modsi3 cagrisina
         * ceviriyor, ROM'da oyle bir cagri yok ve sembol cozulemedigi icin
         * dosya hic derlenmiyordu.  Elle sarmalama ayni anlami veriyor. */
        j = i + 1;
        if (j >= count) j = 0;
        side = ((((point[1] - poly[i][1]) >> 12) * ((poly[j][0] - poly[i][0]) >> 12)
               - ((point[0] - poly[i][0]) >> 12) * ((poly[j][1] - poly[i][1]) >> 12)) << 8);
        side += tolerance;
        if (side < 0) return 0;
    }
    return 1;
}
