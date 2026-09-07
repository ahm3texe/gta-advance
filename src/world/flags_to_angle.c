/* Yon bayraklarini aciya cevirme — 0x08017D78-0x08017E39 (+2 dolgu)
 *
 * 0x2000/0x4000 dikey, 0x100/0x200 yatay bayraklarindan (dx,dy) cikarip
 * sekiz yonu 8.24 sabit noktali aciya esliyor; yon yoksa 0.
 *
 * OLCULEN: ic switch'lerin UCUNDE DE -1/0/1 case'lerinin hepsi yazilmali
 * (0 donenler dahil). Eksik case birakilinca agbcc iki caseli agac
 * kuruyor ve ROM'un `cmp #0 / beq` on testi kayboluyor (47/97 -> 97/97).
 * Ust dallarin ortak `return 0` kuyrugunu derleyici birlestiriyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/flags_to_angle.c
 */

#include "gba_types.h"
#define F_UP 0x2000
#define F_DOWN 0x4000
#define F_LEFT 0x100
#define F_RIGHT 0x200
u32 FlagsToAngle(u32 flags)
{
    s32 dx;
    s32 dy;
    dx = 0;
    dy = 0;
    if (flags & F_UP)    dy = -1;
    if (flags & F_DOWN)  dy = 1;
    if (flags & F_LEFT)  dx = -1;
    if (flags & F_RIGHT) dx = 1;
    switch (dy) {
    case -1:
        switch (dx) {
        case -1: return 0x3800000;
        case 0:  return 0;
        case 1:  return 0x800000;
        }
        break;
    case 0:
        switch (dx) {
        case -1: return 0x3000000;
        case 0:  return 0;
        case 1:  return 0x1000000;
        }
        break;
    case 1:
        switch (dx) {
        case -1: return 0x2800000;
        case 0:  return 0x2000000;
        case 1:  return 0x1800000;
        }
        break;
    }
    return 0;
}
