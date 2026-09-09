/* Decompress a frame and write it to the screen with DMA. 0x08003454, 152 bytes.
 *
 * gRam02001440 holds a {table base, index} pair; every entry in the table is 8
 * bytes: the first word is LZ77-compressed TILE data and the second, when
 * present, is a PALETTE. The tiles are always decompressed and sent to
 * 0x06000040, and the palette, if present, to 0x05000000. The index is then
 * incremented and reset when the end of the table (the entry whose first word
 * is 0) is reached -- so the frames loop.
 *
 * Both transfers happen with interrupts disabled: REG_IME is saved and cleared,
 * then written back after DMA3 is set up. The same pattern is present in
 * flush_palette_queue.c.
 *

 * After the DMA is set up, the control register is READ BACK
 * (`ldr r0,[r4,#8]`). It does not look functional, but it stands in the ROM and
 * skipping it costs four bytes; flush_palette_queue.c has the same line.
 *
 * Ghidra's trailing "Could not recover jumptable" warning is MISLEADING: there
 * is no table there, only the interworking return of the `pop {r0}; bx r0`
 * sequence (rule 35, a void return type). The same mistake occurred with
 * FUN_08017628.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/cutscene_frame.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "ram_symbols.h"

#define TILE_DEST 0x06000040
#define PAL_DEST  0x05000000
#define TILE_CTRL 0x80004B00
#define PAL_CTRL  0x80000080

typedef struct Frame {
    const void *tiles;          /* +0x00 */
    const void *palette;        /* +0x04 */
} Frame;

typedef struct FrameTable {
    Frame *frames;              /* +0x00 */
    s32    index;               /* +0x04 */
} FrameTable;

extern void LZ77UnCompWram(const void *src, void *dst);

void ShowNextCutsceneFrame(void)
{
    FrameTable *table;
    void *buf;
    u16 ime;
    const void *pal;
    s32 i;

    buf = (void *)gRam02001450;
    table = (FrameTable *)gRam02001440;

    LZ77UnCompWram(table->frames[table->index].tiles, buf);
    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = buf;
    REG_DMA3.dst = (void *)TILE_DEST;
    REG_DMA3.control = TILE_CTRL;
    REG_DMA3.control;
    REG_IME = ime;

    pal = table->frames[table->index].palette;
    if (pal != 0) {
        LZ77UnCompWram(pal, buf);
        ime = REG_IME;
        REG_IME = 0;
        REG_DMA3.src = buf;
        REG_DMA3.dst = (void *)PAL_DEST;
        REG_DMA3.control = PAL_CTRL;
        REG_DMA3.control;
        REG_IME = ime;
    }

    /* The ROM increments the index IN PLACE (`adds r0,#1`) and
     * indexes with the INCREMENTED value; producing a separate
     * value as `i + 1` folds the offset into 8 and three
     * instructions diverge. */
    i = table->index;
    i++;
    table->index = i;
    if (table->frames[i].tiles == 0) table->index = 0;
}
