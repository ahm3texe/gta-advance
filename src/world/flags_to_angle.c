/* Convert direction flags to an angle — 0x08017D78-0x08017E39 (+2 padding)
 *
 * Derive (dx,dy) from vertical flags 0x2000/0x4000 and horizontal flags
 * 0x100/0x200; map eight directions to an 8.24 fixed-point angle, or return 0
 * if no direction is set.
 *
 * MEASURED: ALL THREE inner switches must explicitly include -1/0/1 cases,
 * including those returning 0. Missing cases produce a two-case tree and
 * remove the ROM's preliminary cmp #0 / beq (47/97 -> 97/97). The compiler
 * merges the outer branches' shared return-0 tail.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/flags_to_angle.c
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
