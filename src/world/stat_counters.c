/* Istatistik sayaclari — 0x08067228-0x08067273
 *
 * Uc sayac kayit tamponunun icinde duruyor (gSaveBuffer +0x6C, +0x6E,
 * +0x74), yani oyuna kaydediliyorlar. Ilk iki fonksiyon tasma korumali
 * artirim yapiyor: u16 sarmalanip 0 olursa eski deger geri yaziliyor.
 *
 * Ayni kumedeki dorduncu fonksiyon AddDistance ayri dosyada ve
 * byte-matching: src/world/distance_accum.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/stat_counters.c
 */

#include "gba_types.h"

#define ACCUM_MASK   0x1FFF
#define ACCUM_SHIFT  13
#define DELTA_SHIFT  16

/* Kayit tamponunun sayac alanlari; tam yapisi src/world/entity_flags.c
 * ve src/save/save_manager.c'de baska yonleriyle tanimli. */
typedef struct SaveCounters {
    u8  pad00[0x6C];
    u16 countA;                 /* +0x6C */
    u16 countB;                 /* +0x6E */
    u8  pad70[4];
    u16 distance;               /* +0x74 */
} SaveCounters;

extern SaveCounters gSaveBuffer;
extern u32 gDistanceAccum;

extern void Memset(void *dest, int value, u32 size);

/* 0x08067228 */
void BumpCountB(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.countB;
    next = old + 1;
    gSaveBuffer.countB = next;
    if ((u16)next == 0)
        gSaveBuffer.countB = old;
}

/* 0x08067244 */
void BumpCountA(void)
{
    u16 old;
    int next;

    old = gSaveBuffer.countA;
    next = old + 1;
    gSaveBuffer.countA = next;
    if ((u16)next == 0)
        gSaveBuffer.countA = old;
}

/* 0x08067260 */
void ResetDistanceAccum(void)
{
    Memset(&gDistanceAccum, 0, 8);
}
