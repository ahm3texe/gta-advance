/* 65536 girisli tarama — 0x08029014-0x08029051
 *
 * gRam02024650'de 148 baytlik girisli 65536 giris. Her aktif giris icin
 * FUN_080289b8 (subActive!=0) veya FUN_080289b4 cagriliyor.
 *
 * HENUZ ESLESMIYOR: 29 komutun 28'i tutuyor, 2 bayt fark. Tek fark
 * `adds r1, r0, r5` vs `adds r1, r5, r0` — operand sirasi. Toplamda ayni
 * degeri veriyor; C tarafinda ifade sirasini degistirmek etki etmedi
 * (`tbl + i`, `i + tbl`, `(u8*)tbl + i*148` hepsi ayni ciktiyi verdi).
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

extern u32 FUN_080289b8(u16 i);
extern u32 FUN_080289b4(u16 i);

/* 0x08029014 */
void ScanAllEntries(void)
{
    u16 i;
    Entry *e;
    Entry *tbl;

    i = 0;
    tbl = gRam02024650;
    do {
        e = tbl + i;
        if (e->active != 0) {
            if (e->subActive == 0)
                FUN_080289b8(i);
            else
                FUN_080289b4(i);
        }
        i++;
    } while (i == 0);
}
