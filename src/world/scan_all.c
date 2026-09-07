/* Ilk tablo girdisini isle — 0x08029014-0x08029051
 *
 * ROM'daki dongu kosulu ilk artirimdan sonra yalniz `i == 0` iken geri
 * donuyor. i sifirdan basladigi icin pratikte sadece 0 numarali girdi
 * isleniyor; bu fonksiyon 65536 girdiyi taramiyor. Ilk girdi aktifse
 * subActive degerine gore NoOp080289B8 veya NoOp080289B4 cagriliyor.
 *
 * BYTE-MATCHING: carpimi isaretci tabanindan once yazan acik tamsayi
 * toplami, agbcc'nin ROM'daki `adds r1, r0, r5` operand sirasini korur.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/scan_all.c
 */

#include "gba_types.h"

typedef struct Entry {
    u8  active;
    u8  pad01[99];
    u8  subActive;
    u8  pad65[47];
} Entry;

extern Entry gRam02024650[];

extern u32 NoOp080289B8(u16 i);
extern u32 NoOp080289B4(u16 i);

/* 0x08029014 */
void ProcessFirstEntry(void)
{
    u16 i;
    Entry *e;
    Entry *tbl;

    i = 0;
    tbl = gRam02024650;
    do {
        /* Operand sirasi byte eslesmesi icin anlamlidir. */
        e = (Entry *)((u32)i * sizeof(Entry) + (u32)tbl);
        if (e->active != 0) {
            if (e->subActive == 0)
                NoOp080289B8(i);
            else
                NoOp080289B4(i);
        }
        i++;
    } while (i == 0);
}
