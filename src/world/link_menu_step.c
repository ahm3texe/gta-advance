/* Convert a link-menu step (0..35) to a message code: code = 0xB0 + step.
 * Out-of-range steps return 0x93. The ROM uses a 36-entry jump table at
 * 0x08066EF0; each case directly returns its own constant.
 *
 * The range check needs TWO SEPARATE if statements. `if (step < 0 || step > 35)`
 * folds to one unsigned cmp #35 / bls in agbcc, merges with the switch range
 * check, and produces 312 bytes. Separate checks preserve the ROM's signed
 * cmp #0 / blt + cmp #35 / ble, followed by the switch's own cmp #35 / bls.
 */

#include "gba_types.h"

/* 0x08066ED0 */
u32 GetStepIconId(s32 step)
{
    if (step < 0)
        return 0x93;
    if (step > 35)
        return 0x93;

    switch (step) {
    case 0:
        return 0xB0;
    case 1:
        return 0xB1;
    case 2:
        return 0xB2;
    case 3:
        return 0xB3;
    case 4:
        return 0xB4;
    case 5:
        return 0xB5;
    case 6:
        return 0xB6;
    case 7:
        return 0xB7;
    case 8:
        return 0xB8;
    case 9:
        return 0xB9;
    case 10:
        return 0xBA;
    case 11:
        return 0xBB;
    case 12:
        return 0xBC;
    case 13:
        return 0xBD;
    case 14:
        return 0xBE;
    case 15:
        return 0xBF;
    case 16:
        return 0xC0;
    case 17:
        return 0xC1;
    case 18:
        return 0xC2;
    case 19:
        return 0xC3;
    case 20:
        return 0xC4;
    case 21:
        return 0xC5;
    case 22:
        return 0xC6;
    case 23:
        return 0xC7;
    case 24:
        return 0xC8;
    case 25:
        return 0xC9;
    case 26:
        return 0xCA;
    case 27:
        return 0xCB;
    case 28:
        return 0xCC;
    case 29:
        return 0xCD;
    case 30:
        return 0xCE;
    case 31:
        return 0xCF;
    case 32:
        return 0xD0;
    case 33:
        return 0xD1;
    case 34:
        return 0xD2;
    case 35:
        return 0xD3;
    }
    return 0;
}
