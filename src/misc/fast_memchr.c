/* Bellekte bir bayti word-hizli tarayan memchr benzeri yardimci.
 *
 * Hizali ve en az dort baytlik kisimlarda dort kopyali arama kelimesi ile
 * sifir-bayt algilama hilesi kullanilir; kalan baytlar dogrudan taranir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm
 * Dogrulama: make c-match FILE=src/misc/fast_memchr.c
 * PARK: tekrar sabiti ve sifir-bayt hilesinin ifade/scope bicimleri denendi;
 * 136/132 bayt, 83/136 fark. Kalan engel r4-r7 yazmac dagitimi.
 */

#include "gba_types.h"

#define DETECT_ZERO_BYTE(x) (((x) + 0xFEFEFEFF) & ~(x) & 0x80808080)

/* 0x08070D10 */
void *FUN_08070d10(const void *source, s32 value, u32 length)
{
    const u8 *bytes;
    const u32 *words;
    u32 wanted;

    bytes = source;
    words = (const u32 *)bytes;
    wanted = value;
    wanted &= 0xFF;
    if (length > 3 && (((u32)bytes & 3) == 0)) {
        u32 repeated;
        u32 i;
        u32 magic;
        u32 highBits;

        repeated = 0;
        i = 0;
        do {
            repeated = (repeated << 8) + wanted;
            i++;
        } while (i <= 3);
        if (length > 3) {
            magic = 0xFEFEFEFF;
            highBits = 0x80808080;
        }
        while (length > 3) {
            u32 probe;

            probe = *words ^ repeated;
            if (((probe + magic) & ~probe & highBits) != 0) {
                const u8 *scan;

                scan = (const u8 *)words;
                i = 0;
                do {
                    if (*scan == wanted)
                        return (void *)scan;
                    scan++;
                    i++;
                } while (i <= 3);
            }
            length -= 4;
            words++;
        }
        bytes = (const u8 *)words;
    }

    while (length--) {
        if (*bytes == wanted)
            return (void *)bytes;
        bytes++;
    }
    return 0;
}
