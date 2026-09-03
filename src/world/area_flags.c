/* Alan bayraklari — 0x08030C4C-0x08030CB3
 *
 * Kayit tamponunun 0x4C ofsetinde 192 bitlik (alti sozcuk) bir bayrak
 * dizisi var; indeks BIR TABANLI, 0 "yok" demek. src/world/entity_flags.c
 * ayni tampondaki 0x3C'deki 128 bitlik varlik bayraklarini isliyor --
 * oradaki `unk4C[80]` blogunun ilk 24 bayti burada cozuldu.
 *
 * Adres deyimi entity_flags.c'de olculdu: `ldr r0,=taban / adds r0,#N`
 * bicimini yalnizca STRUCT UYESI erisimi uretiyor.
 *
 * Ayni kumedeki dorduncu fonksiyon (CleanupAreaTiles, 0x08030CB4) henuz
 * eslesmedigi icin ayri dosyada: src/world/area_cleanup.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/area_flags.c
 */

#include "gba_types.h"

#define AREA_FLAG_MAX  192

/* Kayit slotu calisma tamponu; 0x3C'deki varlik bayraklari icin
 * src/world/entity_flags.c'ye bakin. */
typedef struct SaveBuffer {
    u8  header[12];             /* 0x00 */
    u8  unk0C[48];              /* 0x0C */
    u32 entityFlags[4];         /* 0x3C — 128 bit */
    u32 areaFlags[6];           /* 0x4C — 192 bit */
    u8  unk64[56];              /* 0x64 */
    u8  complement;             /* 0x9C */
    u8  unk9D[3];
} SaveBuffer;

extern SaveBuffer gSaveBuffer;

/* 0x08030C4C */
u32 IsAreaFlagSet(s32 index)
{
    s32 word;
    s32 bit;
    u32 mask;

    if (index <= 0)
        return 0;

    index -= 1;
    if (index > AREA_FLAG_MAX - 1)
        return 0;

    bit = index & 31;
    mask = 1 << bit;
    word = index >> 5;
    if ((gSaveBuffer.areaFlags[word] & mask) != 0)
        return 1;

    return 0;
}

/* 0x08030C80 */
u32 SetAreaFlag(s32 index)
{
    s32 word;
    s32 bit;
    u32 mask;

    if (index <= 0)
        return 0;

    index -= 1;
    if (index > AREA_FLAG_MAX - 1)
        return 0;

    bit = index & 31;
    mask = 1 << bit;
    word = index >> 5;
    gSaveBuffer.areaFlags[word] |= mask;

    return 1;
}

/* 0x08030CB0 — govdesi bos. */
void AreaFlagsNoop(void)
{
}
