/* Open the menu — 0x0805AAA8-0x0805AADB
 *
 * Mode 0 is the one with a condition: it opens only while the +0x54 word
 * reached through gRam02000F10 is above 15. Every other mode opens
 * unconditionally. Nothing happens at all before gFrameCounterEwram starts.
 *
 * The +0x54 read goes past the 18 bytes data/ram_map.csv records for
 * gRam02000F10, into the gap after player_health at +0x12. The ROM keeps
 * 0x02000F10 in the pool and the 84 as a displacement, so the two are one
 * object as far as the compiler was concerned; the note in data/ram_map.csv
 * says so without changing the symbol, since player_health's own extent is
 * trace-verified.
 *
 * The base goes through a local for the reason src/session/notify_when_pending.c
 * records: applied to the symbol directly the displacement folds into the pool
 * constant (rule 65).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_open_menu.c
 */

#include "gba_types.h"

#define GATE_OFFSET  0x54
#define GATE_MIN     15

extern u32 gFrameCounterEwram;
extern u8  gRam02000F10[];

extern void RunMenuScreen(u32 mode);

/* 0x0805AAA8 */
u32 FUN_0805aaa8(u32 a, u16 mode)
{
    u8 *base;

    if (gFrameCounterEwram == 0)
        return 0;
    if (mode == 0) {
        base = gRam02000F10;
        if (*(s32 *)(base + GATE_OFFSET) <= GATE_MIN)
            return 1;
    }
    RunMenuScreen(mode);
    return 1;
}
