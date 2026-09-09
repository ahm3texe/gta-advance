/* Clearing all the entry tables by DMA — 0x08023CC4-0x08023DEF
 *
 * Zeroes nine separate entry tables (all made of 148-byte entries) with DMA3,
 * and clears two small state variables at the end.  Every block repeats the
 * same pattern: save IME, disable interrupts, use the zero word on the stack
 * as the source and DMA-fill to a fixed address (0x85000000 = enable + 32-bit
 * + fixed source), do a dead read of the control register to wait for the DMA,
 * restore IME.  The pattern is the same as in src/text/clear_text_area.c and
 * src/world/dma_flush.c.
 *
 * The word counts are multiples of 148, i.e. every destination is an entry
 * table:  555=15 entries, 148=4, 37=1, 740=20, 37=1, 148=4, 185=5, 185=5,
 * 37=1.  The first is gEntriesA (0x02023A00, 15 entries x 148 = 2220 bytes) --
 * so this function is the bulk initializer of the gEntriesA family.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/entries_b2.c
 */

#include "gba_io.h"

#define ENTRY_STRIDE   148
#define ENTRIES_A_LEN  15

/* DMA3 control: enable | 32-bit | fixed source; the low 16 bits are the word count. */
#define DMA_FILL_32    0x85000000

#define WORDS_A        555      /* 15 entries */
#define WORDS_B        148      /*  4 entries */
#define WORDS_C         37      /*  1 entry   */
#define WORDS_D        740      /* 20 entries */
#define WORDS_E        185      /*  5 entries */

/* These addresses have NO entry in data/ram_map.csv; since I am not authorized
 * to add any, they are written as constant casts (each address is used on its
 * own, so there is no base + offset folding -- rule 1's rationale does not
 * apply here).  The ones that do have entries are shown in comments. */
#define TABLE_02025280 ((void *)0x02025280)     /* gRam02025280 */
#define TABLE_020245B0 ((void *)0x020245B0)
#define TABLE_020246F0 ((void *)0x020246F0)     /* gRam020246F0 */
#define TABLE_020242B0 ((void *)0x020242B0)
#define TABLE_02024350 ((void *)0x02024350)
#define TABLE_020254D0 ((void *)0x020254D0)
#define TABLE_02023710 ((void *)0x02023710)
#define TABLE_02024650 ((void *)0x02024650)     /* gRam02024650 */

#define STATE_020245A0 (*(u8 *)0x020245A0)
#define STATE_02024344 (*(u16 *)0x02024344)

/* This translation unit's view of gEntriesA: its contents are not used, only
 * the DMA destination and size are needed (see src/world/entries_a1.c). */
typedef struct Entry {
    u8 pad00[ENTRY_STRIDE];
} Entry;

extern Entry gEntriesA[ENTRIES_A_LEN];

/* 0x08023CC4 */
void ClearEntryTables(void)
{
    volatile u32 fill;
    u16 ime;
    volatile DmaChannel *dma;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma = (volatile DmaChannel *)REG_DMA3_ADDR;
    dma->src = &fill;
    dma->dst = gEntriesA;
    dma->control = DMA_FILL_32 | WORDS_A;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02025280;
    dma->control = DMA_FILL_32 | WORDS_B;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020245B0;
    dma->control = DMA_FILL_32 | WORDS_C;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020246F0;
    dma->control = DMA_FILL_32 | WORDS_D;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020242B0;
    dma->control = DMA_FILL_32 | WORDS_C;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02024350;
    dma->control = DMA_FILL_32 | WORDS_B;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_020254D0;
    dma->control = DMA_FILL_32 | WORDS_E;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02023710;
    dma->control = DMA_FILL_32 | WORDS_E;
    dma->control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    dma->src = &fill;
    dma->dst = TABLE_02024650;
    dma->control = DMA_FILL_32 | WORDS_C;
    dma->control;
    REG_IME = ime;

    STATE_020245A0 = 0;
    STATE_02024344 = 0;
}
