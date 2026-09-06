/* On alti 16-bit girdide secilen dort bitlik alanlari degistirir.
 *
 * Her kelimedeki dort nibble ayri ayri eski degerle karsilastirilir. Eslesen
 * alanlar ayni konuma kaydirilmis yeni degerle degistirilir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm
 * Dogrulama: make c-match FILE=src/world/nibble_replace.c
 */

#include "gba_types.h"

/* 0x08031C10 */
void ReplaceNibbleField(const u16 *src, u16 *dst, u32 oldValue, u32 newValue)
{
    int i;
    u32 mask;
    u32 n4;
    u32 n8;
    u32 n12;
    const u16 *read;
    u16 *write;

    mask = 15;
    n4 = newValue << 4;
    n8 = newValue << 8;
    n12 = newValue << 12;
    write = dst;
    read = src;
    i = 15;
    do {
        u32 value;

        value = *read;
        if ((value & mask) == oldValue)
            value = (value & 0xFFF0) | newValue;
        if (((value >> 4) & mask) == oldValue)
            value = (value & 0xFF0F) | n4;
        if (((value >> 8) & mask) == oldValue)
            value = (value & 0xF0FF) | n8;
        if (((value >> 12) & mask) == oldValue)
            value = (value & 0x0FFF) | n12;
        *write = value;
        write++;
        read++;
        i--;
    } while (i >= 0);
}
