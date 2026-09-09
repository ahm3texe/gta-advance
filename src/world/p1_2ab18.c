/* Draw the HUD minute and second tiles; the sibling DrawTwoDigits idiom — 0x0802AB18.
 * Phase 1; compared against the ROM twins. Measurement: data/phase1_evidence/0x0802AB18.json. */
#include "gba_types.h"
#include "ram_symbols.h"

extern s32 Div(s32,s32);
void FUN_0802ab18(s32 minutes,s32 seconds)
{
    s32 digits[8];
    vu16 *top,*bottom;
    s32 d,i,j;
    if (minutes < 0) minutes = 0;
    if (seconds < 0) seconds = 0;
    if (minutes > 99) minutes = 99;
    if (seconds > 59) seconds = 59;
    gRam02025810[6] = minutes;
    gRam02025810[7] = seconds;
    digits[1] = Div(minutes,10);
    digits[0] = minutes - digits[1]*10;
    if (digits[1] == 0) digits[1] = 10;
    top = (vu16 *)0x6009954;
    bottom = (vu16 *)0x6009994;
    for (i = 0; i < 2; i++) {
        d = digits[i];
        if (d == 10) {
            *top-- = 0xF0E8;
            *bottom-- = 0xF0E8;
        } else {
            *top-- = (248 + d) | 0xF000;
            *bottom-- = (258 + d) | 0xF000;
        }
    }
    digits[1] = Div(seconds,10);
    digits[0] = seconds - digits[1]*10;
    top = (vu16 *)0x600995a;
    bottom = (vu16 *)0x600999a;
    for (j = 0; j < 2; j++) {
        d = digits[j];
        if (d == 10) {
            *top-- = 0xF0E8;
            *bottom-- = 0xF0E8;
        } else {
            *top-- = (248 + d) | 0xF000;
            *bottom-- = (258 + d) | 0xF000;
        }
    }
    *(vu16 *)0x6009956 = 0xF0ED;
}
