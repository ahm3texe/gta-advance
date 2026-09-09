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
 * Verification: make c-match FILE=src/ui/load_hud_palettes.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* 0x08030EAC — 124 bytes, BYTE-MATCHING.
 *
 * Initialize two palette blocks. Clear flag 0x02026BE0, disable interrupts,
 * DMA3-fill 16 halfwords at 0x0600A780 from stack constant 0x3333 with fixed
 * source (0x81000010). Restore interrupts, CpuSet-copy 0x08347C28 ->
 * 0x0600A7A0, then DMA3-copy 32 halfwords 0x08348B28 -> 0x0600A7C0.
 *
 * MEASURED:
 * - fill must be on the stack and volatile (rule 3); otherwise agbcc reorders
 *   address formation and constant loading.
 * - Constant directly in the store: only HImode produces ldr r3,=0x3333 /
 *   adds r0,r3,#0 (hud_fields.c mechanism 1).
 * - REG_DMA3.control reads are required readbacks, not dispensable dead reads:
 *   ROM ldr r0,[r4,#8] (rule 62, volatile view).
 * - 0x02026BE0 lacked a ram_map record. Its zero-offset strb r0,[r1,#0]
 *   makes a cast reproduce the bytes; rule 1 does not intervene. Prefer
 *   extern u8 gRam02026BE0 if the symbol is registered.
 */

#define PAL_FLAG      (*(u8 *)0x02026BE0)
#define PAL_FILL      0x3333
#define PAL_FILL_DST  ((void *)0x0600A780)
#define PAL_SRC       ((const void *)0x08347C28)
#define PAL_DST       ((void *)0x0600A7A0)
#define PAL_SRC2      ((const void *)0x08348B28)
#define PAL_DST2      ((void *)0x0600A7C0)
#define DMA_FILL16    0x81000010        /* enable, fixed source, 16 halfwords */
#define DMA_COPY16    0x80000020        /* enable, 32 halfwords */
#define CPUSET_COPY   0x30

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x08030EAC */
void LoadHudPalettes(void)
{
    vu16 fill;
    u16 ime;

    PAL_FLAG = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = PAL_FILL;
    REG_DMA3.src = (const void *)&fill;
    REG_DMA3.dst = PAL_FILL_DST;
    REG_DMA3.control = DMA_FILL16;
    REG_DMA3.control;
    REG_IME = ime;

    CpuSet(PAL_SRC, PAL_DST, CPUSET_COPY);

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = PAL_SRC2;
    REG_DMA3.dst = PAL_DST2;
    REG_DMA3.control = DMA_COPY16;
    REG_DMA3.control;
    REG_IME = ime;
}

