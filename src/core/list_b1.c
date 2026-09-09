/* Clear the list head and two blocks — 0x0800DB80-0x0800DBE7
 *
 * It first clears the list head at 0x02016280, then fills two blocks with DMA3:
 * 0x11F8 words (18400 bytes) at 0x02016290 and 3 words at 0x0201AA80. Each DMA
 * sits inside its own REG_IME save/restore pair.
 * At the end it clears the byte at 0x0201AA9C.
 *
 * The three addresses in the pool (0x02016290, 0x0201AA80, 0x0201AA9C) are
 * loaded as SEPARATE literals in the ROM, not as base+offset -- which is why
 * they were written as separate objects. None of the three is in
 * data/ram_map.csv; they were reached through raw address casts (rule 1's
 * folding trap does not arise here, because none of them is a member of a
 * shared base).
 *
 * `str r3,[sp]` appears twice -> the stack temporary must be `volatile`,
 * otherwise the second zero store is eliminated as a dead value (rule 3).
 * `movs r5,#0` is a separate zero: because a byte store is QImode it falls to
 * its own pseudo and does not merge with the u32 zero.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * MATCH: 104/104 bytes, on the first attempt. The template is the
 * REG_IME + REG_DMA3 pattern from src/core/init_sprite_pool.c and
 * src/world/dma_flush.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/list_b1.c
 */

#include "gba_io.h"

#define DMA_FILL_BIG    0x850011F8
#define DMA_FILL_SMALL  0x85000003

#define gBuf02016290   ((void *)0x02016290)
#define gBuf0201AA80   ((void *)0x0201AA80)
#define gFlag0201AA9C  (*(u8 *)0x0201AA9C)

extern void *gListHead02016280;

/* 0x0800DB80 */
void ResetListHead(void)
{
    u16 ime;
    volatile u32 zero;

    gListHead02016280 = 0;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gBuf02016290;
    REG_DMA3.control = DMA_FILL_BIG;
    (void)REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gBuf0201AA80;
    REG_DMA3.control = DMA_FILL_SMALL;
    (void)REG_DMA3.control;
    REG_IME = ime;

    gFlag0201AA9C = 0;
}
