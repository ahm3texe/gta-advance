/* Menu ekrani baslatma — 0x080013AC-0x08001457
 *
 * Blend register'larini ayarlar, palette verisini ve VRAM temizligini DMA3
 * ile yapar, menuyle iliskili alt sistemleri baslatir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/init_menu_screen.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct {
    const void *src;
    void *dst;
    u32 control;
} DmaChannel;

#define REG_BLDCNT    (*(u16 *)0x04000050)
#define REG_BLDALPHA  (*(u16 *)0x04000052)
#define REG_IME       (*(volatile u16 *)0x04000208)
#define REG_DMA3      (*(volatile DmaChannel *)0x040000D4)

#define PALETTE_RAM   ((void *)0x05000000)
#define VRAM          ((void *)0x06000000)
#define DMA_COPY_PALETTE 0x80000100
#define DMA_CLEAR_VRAM   0x81004B00

#define BLEND_ALL     0xFF
#define MENU_RESOURCE 0x11B

#define gIwramFrameCounter (*(u32 *)0x03000004)

extern u8 gMenuPaletteSource[];

extern void FUN_08063cf0(void);
extern void FUN_080353bc(void);
extern void FUN_08012248(void);
extern void FUN_080104c0(void);
extern void FUN_0800dd50(void);
extern void FUN_0802f55c(void);
extern void FUN_0800ab6c(void);
extern void FUN_0803493c(void);
extern void FUN_08063b98(void);
extern u8 *FUN_0803c49c(void);
extern void FUN_080348b4(u32 resource);

/* 0x080013AC */
void InitMenuScreen(void)
{
    volatile u16 fill;
    u16 ime;
    u8 *entry;
    /* Basta yerel degiskene alinir: ROM bu adresi cagrilar boyunca r5'te
     * tutuyor. Kullanildigi yerde okunursa derleyici hoist etmiyor. */
    const u8 *palette = gMenuPaletteSource;

    FUN_08063cf0();
    FUN_080353bc();
    FUN_08012248();

    REG_BLDCNT = BLEND_ALL;
    REG_BLDALPHA = 0;

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = palette;
    REG_DMA3.dst = PALETTE_RAM;
    REG_DMA3.control = DMA_COPY_PALETTE;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = &fill;
    REG_DMA3.dst = VRAM;
    REG_DMA3.control = DMA_CLEAR_VRAM;
    REG_DMA3.control;
    REG_IME = ime;

    FUN_080104c0();
    FUN_0800dd50();
    FUN_0802f55c();
    FUN_0800ab6c();
    FUN_0803493c();
    FUN_08063b98();

    gIwramFrameCounter = 1;

    entry = FUN_0803c49c();
    if (entry == 0)
        return;
    if (entry[55] == 0)
        return;

    FUN_080348b4(MENU_RESOURCE);
}
