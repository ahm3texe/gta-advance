/* Refresh timers for four-condition matches in gEntriesA — 0x0802929C-0x080292EB
 *
 * Forward the parameter to FUN_0803535C, then scan all 15 gEntriesA entries
 * (stride 148): active at +0x00, +0x84 equals the parameter, type +0x64 is 34,
 * and +0x90 is 39. Write 20 to the +0x02 halfword of every matching entry;
 * there is no early exit.
 *
 * Rule 35: `pop {r0}; bx r0` indicates void. Its sibling FindEntryByQuad
 * (entries_a4.c) uses the same loop skeleton. Array indexing `gEntriesA[i].field`
 * produces the ROM's (base+offset)+i*148 address association.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_a3.c
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
