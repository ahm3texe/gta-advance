/* Historical band B: nine functions from 0x08030B34 .. 0x08031D23.
 *
 * MEASUREMENT NOTE: results from the former src/world/band_b.c were misleading.
 * agbcc_build.py links all functions consecutively from min(address), using
 * SUBALIGN(1). These nine are not contiguous in the ROM: other translation
 * units intervene. All but the first (SendTextMode1) were linked at a fixed
 * displacement from their ROM addresses, making every bl offset wrong even
 * for otherwise matching code (reported as 2/N differing bytes).
 * Each was therefore measured separately at its own ROM address:
 *   SendTextMode1 12 BYTE-MATCHING
 *   LoadHudPalettes 124 BYTE-MATCHING
 *   TriggerEvent39 12 BYTE-MATCHING
 *   GetRecordNodeById 14 BYTE-MATCHING
 *   ScaleMagnitude 132 BYTE-MATCHING
 *   ClearHudRowsAB 72 BYTE-MATCHING
 *   ReleaseActorAndSlot 64 BYTE-MATCHING
 *   BlitStripClipLeft4bpp 470 NON-MATCHING
 *   PushSlotQueueEntry 140 MISSING RAM SYMBOL
 * These are historical results; detailed notes accompany the split sources.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm (docs/COMPILER.md)
 * Verification: make c-match FILE=src/ui/clear_hud_rows_ab.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* 0x08031578 — 72 bytes, BYTE-MATCHING.
 *
 * Fill four VRAM tile rows with empty tile 0xF0E8 using two eight-halfword
 * loops with pointers walking BACKWARDS. Same pattern as ClearHudFieldA:
 * - Tile constant directly in the store: HImode store emits ldr + adds copy;
 *   a local removes the copy (mechanism 1).
 * - Write an INCREASING counter; agbcc reverses the loop and initializes it
 *   after the hoisted constant (mechanism 2).
 * The lifetime-extending a++; a--; no-op from ClearHudFieldA is unnecessary:
 * plain source already gives the ROM allocation a=r2, tile=r3.
 */

#define TILE_BLANK 0xF0E8
#define TILE_RUN   8

#define ROW_A_LEFT  ((vu16 *)0x060099BA)
#define ROW_A_RIGHT ((vu16 *)0x060099FA)
#define ROW_B_LEFT  ((vu16 *)0x06009A3A)
#define ROW_B_RIGHT ((vu16 *)0x06009A7A)

/* 0x08031578 */
void ClearHudRowsAB(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = ROW_A_LEFT;
    b = ROW_A_RIGHT;
    for (i = 0; i < TILE_RUN; i++) {
        *a = TILE_BLANK;
        a--;
        *b = TILE_BLANK;
        b--;
    }

    a = ROW_B_LEFT;
    b = ROW_B_RIGHT;
    for (i = 0; i < TILE_RUN; i++) {
        *a = TILE_BLANK;
        a--;
        *b = TILE_BLANK;
        b--;
    }
}

