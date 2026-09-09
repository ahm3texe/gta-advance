/* Set the timer, from an operand and from the clock
 * 0x0805A450-0x0805A493 and 0x0805A494-0x0805A4F3
 *
 * Two adjacent handlers with the same tail. Both turn a count of seconds into
 * minutes and seconds and hand the pair to FUN_0802A9F0, then clear the HUD
 * time, notify the active slot with id 355 and call FUN_08032318.
 *
 * They differ only in where the count comes from: the first takes the operand
 * as it stands, the second adds it to the current clock, which
 * src/progress/read_signed_pair.c reads out of the progress block as a signed
 * minutes and seconds pair.
 *
 * The clock total stays a WIDE type until the operand has been added, and is
 * narrowed to 16 bits only once, at the end. Kept in a u16 throughout it is
 * narrowed twice, which is two instructions the ROM does not have.
 *
 * The operand is added to the clock in a SEPARATE statement. Written as one
 * expression agbcc reassociates it, adding the operand to the seconds first and
 * the product afterwards, which is two instructions in the wrong order and
 * loses the callee-saved register the ROM keeps the operand in across the call.
 *
 * The division is UNSIGNED (`__udivsi3`), and the remainder is computed as
 * `total - minutes * 60` rather than with a second division; the ROM builds the
 * 60 as `(q << 4) - q` shifted twice, which is agbcc's expansion of that
 * multiply and not something the source says.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_set_timer.c
 */

#include "gba_types.h"

#define SECONDS_PER_MINUTE  60
#define NOTIFY_ID           355

extern void FUN_0802a9f0(s32 minutes, s32 seconds);
extern void SetHudTime(s32 minutes, s32 seconds);
extern void FUN_08030b88(s32 *minutes, s32 *seconds);
extern u32  GetActiveSlot(void);
extern void FUN_08035058(u32 slot, u32 id);
extern void FUN_08032318(void);

/* 0x0805A450 */
u32 FUN_0805a450(u32 a, u16 seconds)
{
    u16 total = seconds;
    u16 minutes;
    u32 rest;

    minutes = total / SECONDS_PER_MINUTE;
    rest = total - minutes * SECONDS_PER_MINUTE;
    FUN_0802a9f0(minutes, rest);
    SetHudTime(0, 0);
    FUN_08035058(GetActiveSlot(), NOTIFY_ID);
    FUN_08032318();
    return 1;
}

/* 0x0805A494 */
u32 FUN_0805a494(u32 a, u16 extra)
{
    s32 clockMinutes;
    s32 clockSeconds;
    s32 sum;
    u16 total;
    u16 minutes;
    u32 rest;

    FUN_08030b88(&clockMinutes, &clockSeconds);
    sum = clockMinutes * SECONDS_PER_MINUTE + clockSeconds;
    sum += extra;
    total = sum;
    minutes = total / SECONDS_PER_MINUTE;
    rest = total - minutes * SECONDS_PER_MINUTE;
    FUN_0802a9f0(minutes, rest);
    SetHudTime(0, 0);
    FUN_08035058(GetActiveSlot(), NOTIFY_ID);
    FUN_08032318();
    return 1;
}
