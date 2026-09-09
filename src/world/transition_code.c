/* Map an object's previous and new size classes to a transition code.
 * PARKED: tried the switch and if/else forms. Latest measurement: 168/154
 * bytes, 149/168 differing; the ROM orders its comparison tree differently
 * from what natural C produces.
 */

#include "gba_types.h"

typedef struct TransitionCodeState {
    u8 pad00[0x16];
    u8 code;
    u8 direction;
} TransitionCodeState;

/* 0x08014E48 */
void FUN_08014e48(TransitionCodeState *state, u32 from, u32 to)
{
    u8 *fields;
    u8 code;

    fields = (u8 *)state + 0x10;
    if (from == to) {
        fields[7] = 0;
        switch (from) {
        case 8:
            state->code = 0;
            return;
        case 0x10:
            code = 1;
            break;
        case 0x20:
            code = 2;
            break;
        case 0x40:
            code = 3;
            break;
        default:
            return;
        }
    } else if (to < from) {
        fields[7] = 1;
        switch (from) {
        case 0x10:
            code = 4;
            break;
        case 0x20:
            switch (to) {
            case 8:
                code = 5;
                break;
            case 0x10:
                code = 6;
                break;
            default:
                return;
            }
            break;
        case 0x40:
            code = 7;
            break;
        default:
            return;
        }
    } else {
        fields[7] = 2;
        switch (to) {
        case 0x10:
            code = 8;
            break;
        case 0x20:
            switch (from) {
            case 8:
                code = 9;
                break;
            case 0x10:
                code = 10;
                break;
            default:
                return;
            }
            break;
        case 0x40:
            code = 11;
            break;
        default:
            return;
        }
    }
    fields[6] = code;
}
