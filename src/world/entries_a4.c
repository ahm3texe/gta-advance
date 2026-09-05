/* gEntriesA tablosunda dortlu esleme aramasi — 0x08029244-0x08029299
 *
 * gEntriesA'daki 15 giriste (stride 148) sirayla su dortlu kosulu arar:
 * giris aktif (+0x00), +0x84 alani birinci parametreye esit, +0x64 turu 34,
 * +0x90 alani ikinci parametreye esit. Bulursa 1, bulamazsa 0 doner.
 *
 * BYTE-MATCHING (ilk denemede). ROM her turda `i * 148` carpimini yeniden
 * yapiyor; kardes kind_scan.c'deki isaretci yurumesi DEGIL, duz dizi indeksi
 * bicimi dogru olan. Dort ayri alan dort ayri taban gerektirdigi icin agbcc
 * guclendirmeye (strength reduction) gitmiyor, carpimi dongude birakiyor.
 * Sayac `s32`; ROM'un `cmp r1,#14` + `ble` isaretli dali bunu gerektiriyor
 * (kural 9/31).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a4.c
 */

#include "gba_types.h"

#define WANTED_KIND   34

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01[99];
    u8  kind;                   /* +0x64 */
    u8  pad65[31];
    u32 unk84;                  /* +0x84 */
    u8  pad88[8];
    u32 unk90;                  /* +0x90, stride 148 */
} Entry;

extern Entry gEntriesA[];

/* 0x08029244 */
u32 FUN_08029244(u32 a, u32 b)
{
    s32 i;

    for (i = 0; i <= 14; i++) {
        if (gEntriesA[i].active != 0
         && gEntriesA[i].unk84 == a
         && gEntriesA[i].kind == WANTED_KIND
         && gEntriesA[i].unk90 == b)
            return 1;
    }

    return 0;
}
