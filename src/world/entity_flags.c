/* Varlik bayrak sorgusu — 0x08032058-0x0803208F
 *
 * HENUZ ESLESMIYOR: 50 byte'lik ROM fonksiyonuna karsi 52 byte uretiyoruz,
 * 43 bayt farkli. Yapisi dogru cikarildi ve iki ayrinti olculdu:
 *   - Indeks kaydirmasi ISARETLI olmali (ROM asrs kullaniyor) -> s32 index
 *   - Sinir karsilastirmasi ISARETSIZ olmali (ROM bhi kullaniyor)
 * Cozulemeyen: ROM taban adresini ayri yukleyip 60 EKLIYOR
 * (ldr r0,=0x02000D50 / adds r0,#60), bizimki ikisini tek literale katliyor.
 * Denenen bicimler: &dizi[60], (u8*)+60, yerel taban isaretcisi (56B/21 fark),
 * struct uyesi (56B/21 fark). Hicbiri 50 bayta inmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entity_flags.c
 */

#include "gba_types.h"

typedef struct {
    u8  unk00[22];
    u16 id;         /* 0x16 — bir tabanli; 0 "yok" demek */
} Entity;

/* Kayit slotu tamponunun 60. byte'indan itibaren bir bit alani duruyor;
 * gSaveBuffer save_manager tarafindan da kullaniliyor. */
#define SAVE_FLAG_BITS  60
#define ENTITY_ID_MAX   128

extern u8 gSaveBuffer[160];

/* 0x08032058 */
u32 IsEntityFlagSet(const Entity *entity)
{
    s32 index;
    u32 *bits;

    if (entity->id == 0)
        return 0;

    index = (u16)(entity->id - 1);
    if ((u32)index > ENTITY_ID_MAX - 1)
        return 0;

    bits = (u32 *)&gSaveBuffer[SAVE_FLAG_BITS];
    if (bits[index >> 5] & (1 << (index & 31)))
        return 1;

    return 0;
}
