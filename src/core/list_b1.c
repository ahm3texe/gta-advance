/* Liste basini ve iki bloku sifirlama — 0x0800DB80-0x0800DBE7
 *
 * Once 0x02016280'deki liste basini sifirliyor, sonra DMA3 ile iki blok
 * dolduruyor: 0x02016290'a 0x11F8 kelime (18400 bayt) ve 0x0201AA80'e
 * 3 kelime. Her DMA kendi REG_IME kaydet/geri-yukle ciftinin icinde.
 * Sonda 0x0201AA9C baytini sifirliyor.
 *
 * Havuzdaki uc adres (0x02016290, 0x0201AA80, 0x0201AA9C) ROM'da AYRI
 * literal olarak yukleniyor, taban+ofset olarak degil -- bu yuzden ayri
 * nesneler olarak yazildi. Ucu de data/ram_map.csv'de YOK; ham adres
 * cast'i ile gecildi (kural 1'in katlanma tuzagi burada olusmuyor,
 * cunku hicbiri ortak bir tabanin uyesi degil).
 *
 * `str r3,[sp]` iki kez cikiyor -> yigin gecicisi `volatile` olmali,
 * yoksa ikinci sifir yazimi olu deger olarak eleniyor (kural 3).
 * `movs r5,#0` ayri bir sifir: byte yazimi QImode oldugu icin kendi
 * pseudo'suna dusuyor, u32 sifiriyla birlesmiyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * ESLESME: 104/104 bayt, ilk denemede. Sablon src/core/init_sprite_pool.c
 * ve src/world/dma_flush.c'deki REG_IME + REG_DMA3 kalibi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/list_b1.c
 */

#include "gba_io.h"

#define DMA_FILL_BIG    0x850011F8
#define DMA_FILL_SMALL  0x85000003

#define gBuf02016290   ((void *)0x02016290)
#define gBuf0201AA80   ((void *)0x0201AA80)
#define gFlag0201AA9C  (*(u8 *)0x0201AA9C)

extern void *gListHead02016280;

/* 0x0800DB80 */
void FUN_0800db80(void)
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
