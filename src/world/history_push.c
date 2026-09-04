/* Gecmis kuyruguna ekleme — 0x08008064-0x08008093
 *
 * gHistory'nin +0'inda guncel kayit, +0x78'de asagi dogru kaydirilan
 * 31 girisli gecmis var. Yeni kayit guncelden farkliysa kuyruk bir
 * eleman geriye kaydirilip guncel yenileniyor.
 *
 * Kaydirma GERIYE dogru yuruyor (yuksek adresten alcaga), yani kuyruk
 * son elemani ezerek asagi iniyor.
 *
 * BYTE-MATCHING. `current` degerini ayri okuyup `h2 = h` taban kopyasini
 * karsilastirmadan once yapmak ROM'daki r0 -> r4 yasam araligini korur ve
 * esitlik dalinin sondaki store ile birlestirilmesini engeller.
 *
 * Kardesi SetIndexReturnOne: src/world/set_index.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/history_push.c
 */

#include "gba_types.h"

#define HISTORY_LAST   30
#define HISTORY_HEAD   0x78

typedef struct History {
    u32 current;                /* +0x00 */
    u8  pad04[HISTORY_HEAD - 4];
    u32 slots[31];              /* +0x78 */
} History;

extern History gHistory;

/* 0x08008064 */
void PushHistory(u32 value)
{
    History *h;
    History *h2;
    u32     *cur;
    u32      current;
    s32      i;

    if (value == 0)
        return;

    h = &gHistory;
    current = h->current;
    h2 = h;
    if (value == current)
        return;

    i = HISTORY_LAST;
    cur = h2->slots;

    do {
        cur[1] = cur[0];
        cur--;
        i--;
    } while (i >= 0);

    h2->current = value;
}
