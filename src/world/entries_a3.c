/* gEntriesA'da dortlu esleme bulup zamanlayici tazeleme — 0x0802929C-0x080292EB
 *
 * Once parametreyi FUN_0803535C'ye iletiyor, sonra gEntriesA'daki 15 giriste
 * (stride 148) su dortlu kosulu ariyor: giris aktif (+0x00), +0x84 alani
 * parametreye esit, +0x64 turu 34, +0x90 alani 39. Uyan her giriste +0x02
 * yarim sozune 20 yaziyor. Erken cikis yok, tum tabloyu geziyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Kardesi FindEntryByQuad (src/world/entries_a4.c) ayni dongu iskeletini
 * kullaniyor; `gEntriesA[i].alan` dizi-indeks bicimi ROM'un
 * (taban+ofset)+i*148 adres iliskilendirmesini uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a3.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE  148
#define WANTED_KIND   34
#define WANTED_PHASE  39
#define TIMER_RESET   20

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01;
    u16 timer;                  /* +0x02 */
    u8  pad04[96];
    u8  kind;                   /* +0x64 */
    u8  pad65[31];
    u32 unk84;                  /* +0x84 */
    u8  pad88[8];
    u32 unk90;                  /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_0803535c(u32 a);

/* 0x0802929C */
void RefreshEntryTimerByQuad(u32 a)
{
    s32 i;

    FUN_0803535c(a);

    for (i = 0; i <= 14; i++) {
        if (gEntriesA[i].active != 0
         && gEntriesA[i].unk84 == a
         && gEntriesA[i].kind == WANTED_KIND
         && gEntriesA[i].unk90 == WANTED_PHASE)
            gEntriesA[i].timer = TIMER_RESET;
    }
}
