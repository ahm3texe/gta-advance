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
 * Verification: make c-match FILE=src/world/get_record_node_by_id.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* ---- 0x08031318 — 14 bytes, BYTE-MATCHING -------------------------------
 *
 * Forwards a 16-bit id to GetOrCreateRecordNode and returns its result.
 *
 * TWO MEASUREMENTS:
 *  - `pop {r1}; bx r1` (NOT r0) -> the return value is used, i.e. the function
 *    returns the call's result.  The reverse direction of rule 35.
 *  - The leading `lsls r0,#16 / lsrs r0,#16` pair comes from the PARAMETER
 *    being u16.  Writing `id & 0xFFFF` instead loads 0xFFFF from the pool and
 *    produces `ands` (measured: 20 bytes, 3/9 instructions).
 */

typedef struct Node Node;

extern Node *GetOrCreateRecordNode(s32 id);

/* 0x08031318 */
Node *GetRecordNodeById(u16 id)
{
    return GetOrCreateRecordNode(id);
}

