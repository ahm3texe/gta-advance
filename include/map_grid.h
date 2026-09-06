#ifndef GUARD_MAP_GRID_H
#define GUARD_MAP_GRID_H

#include "gba_types.h"

/* Harita izgarasi — gRam0201AEE8'in gosterdigi yapi.
 *
 * Yerlesim 0x080651E0'dan OLCULDU: fonksiyon +0x00 ve +0x02'yi sinir
 * kontrolunde u16 olarak okuyor, +0x04'teki isaretciyi `y * width + x`
 * ile indeksleyip u16 karo cekiyor.
 *
 * Iki kaynak bu sembolu kullaniyor:
 *   src/world/slot_probe.c    izgarayi okuyor (alanlara erisiyor)
 *   src/world/window_config.c yalnizca isaretciyi yaziyor
 * Ayni sembol icin farkli extern turleri tutarlilik denetimini
 * durduruyor; tek tanim burada.
 */
typedef struct Grid {
    u16  width;                 /* +0x00 */
    u16  height;                /* +0x02 */
    u16 *tiles;                 /* +0x04 */
} Grid;

extern Grid *gRam0201AEE8;

#endif /* GUARD_MAP_GRID_H */
