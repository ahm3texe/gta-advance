/* Iki seviyeyi kirpip uygulama — 0x0803C0E4-0x0803C177
 *
 * Bayragin 0. biti kuruluysa gRam02000F10 +0x0C'ye, 1. biti kuruluysa
 * (ve oyun durumu etkinse) gRam02001140 +0x0C'ye degeri yaziyor. Her
 * ikisinde de deger [0, 0x10000] araligina kirpiliyor, sonra secici
 * uygun degerdeyse FUN_08030A60'a olceklenmis hali veriliyor.
 *
 * ONEMLI ASIMETRI: secici (gSlotSelector) BIRINCI blokta ISARETLI
 * okunuyor (`ldrsh`, != 0 sinamasi), IKINCI blokta ISARETSIZ (`ldrh`,
 * == 1 sinamasi). Ikisini ayni yazmak farkli yukleme komutu uretir.
 *
 * Ust sinir `0x80 << 9` ile kuruluyor; duz 0x10000 havuz yuklemesi
 * uretirdi.
 *
 * gRam02000F10 ve gRam02001140 paylasilan ham depolama
 * (include/ram_symbols.h); yerelde cast ediliyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/apply_two_levels.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define FLAG_FIRST  1
#define FLAG_SECOND 2
#define LEVEL_MAX   (0x80 << 9)
#define SCALE       100

typedef struct Level {
    u8  pad00[12];
    s32 value;                  /* +0x0C */
} Level;

typedef struct GameState {
    u8 pad00[12];
    u8 flag;                    /* +0x0C */
} GameState;

extern u16 gSlotSelector;

/* Ayni sembol bu fonksiyonda IKI FARKLI isaretlilikle okunuyor: birinci
   blokta `ldrsh` (isaretli), ikincide `ldrh` (isaretsiz). Satir ici
   `(s16)` cast'i ve volatile okuma AGBCC TARAFINDAN ATILIYOR (sifirla
   karsilastirmada isaret gereksiz sayiliyor, 144 bayt cikiyor); ayri bir
   makro bildirimi korunuyor ve `movs r2,#0` + `ldrsh r0,[r0,r2]` ciftini
   uretiyor. */
#define gSlotSelectorSigned (*(s16 *)&gSlotSelector)
extern GameState gGameState;

extern void FUN_08030a60(s32 scaled, u32 arg);

/* 0x0803C0E4 */
void ApplyTwoLevels(s32 value, u32 flags)
{
    Level *level;

    if (flags & FLAG_FIRST) {
        level = (Level *)gRam02000F10;
        level->value = value;
        if (value > LEVEL_MAX)
            level->value = LEVEL_MAX;
        if (level->value < 0)
            level->value = 0;
        if (gSlotSelectorSigned == 0)
            FUN_08030a60((SCALE * level->value) >> 16, 1);
    }

    if (flags & FLAG_SECOND) {
        if (gGameState.flag != 0) {
            level = (Level *)gRam02001140;
            level->value = value;
            if (value > LEVEL_MAX)
                level->value = LEVEL_MAX;
            if (level->value < 0)
                level->value = 0;
            if (gSlotSelector == 1)
                FUN_08030a60((SCALE * level->value) >> 16, 1);
        }
    }
}
