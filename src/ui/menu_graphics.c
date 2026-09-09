/* Menu graphics — 0x08001E30-0x08001F03
 *
 * Expand menu graphics into VRAM, DMA3-load two palette ranges, fill VRAM
 * and apply DISPCNT settings.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/menu_graphics.c
 */

#include "gba_io.h"



#define MENU_TILES_A   ((const void *)0x08831880)
#define MENU_TILES_B   ((const void *)0x0883F880)
#define MENU_PALETTE_0 ((const void *)0x08831680)
#define MENU_PALETTE_1 ((const void *)0x08EC7A44)
#define PALETTE_DEST_0 ((void *)0x05000140)
#define PALETTE_DEST_1 ((void *)0x05000180)

#define DMA_COPY_PALETTE 0x80000010
#define DMA_CLEAR_VRAM   0x81004B00
#define VRAM_FILL_VALUE  0x9090
#define MENU_DISPCNT     0x0101

/* 0x0806430C — the assembly source called this LoadGraphicsResource.
 * At the time of this note its body was unreviewed, so the Ghidra name
 * was retained pending verification.
 */
extern void SetTextContext(void *dest, u32 tileCount,
                         const void *tilesA, const void *tilesB,
                         u32 width, u32 flags);

/* 0x08001E30 */
void LoadMenuGraphics(u32 offset)
{
    u16 ime;

    SetTextContext((void *)(VRAM_BASE + offset), 30,
                 MENU_TILES_A, MENU_TILES_B, 160, 0);

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_0;
    REG_DMA3.dst = PALETTE_DEST_0;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = MENU_PALETTE_1;
    REG_DMA3.dst = PALETTE_DEST_1;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;
}

/* 0x08001EA0 */
void ClearMenuVram(void)
{
    volatile u16 fill;
    u16 ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = VRAM_FILL_VALUE;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = (void *)VRAM_BASE;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;
    REG_IME = ime;
}

/* 0x08001EDC */
void EnableMenuDisplay(void)
{
    REG_DISPCNT = MENU_DISPCNT;
}

/* 0x08001EF0 */
void EnableMenuDisplayAlt(void)
{
    REG_DISPCNT = MENU_DISPCNT;
}
