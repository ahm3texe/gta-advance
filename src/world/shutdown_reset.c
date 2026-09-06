/* Donanimi kapatip yumusak sifirlama — 0x08065650-0x080656F3
 *
 * Dort DMA kanalinin denetim alanini iki adimda temizliyor (once
 * tekrar/zamanlama bitleri, sonra etkinlestirme biti), her kanaldan sonra
 * bir defa OKUYOR -- donanimda yazimin oturmasi icin gereken bos okuma,
 * bu yuzden OKUMA gorunumu volatile olmali. Ardindan karisim, kesme,
 * ekran ve dort arka plan denetim yazmacini sifirlayip BIOS yumusak
 * sifirlamasini cagiriyor.
 *
 * DURUM: BYTE-MATCHING — 164/164 bayt (0x08065650-0x080656F3).
 *
 * Iki kol kapatti (ikisi de yeni kural adayi):
 *
 * 1) DMA denetim yazmacinda YAZIM gorunumu volatile OLMAMALI, OKUMA
 *    gorunumu volatile kalmali. `volatile` bir lvalue'ya yazarken agbcc
 *    `strh`den hemen once olu bir `ldrh` uretiyor; dort kanalda sekiz
 *    fazla komut (16 bayt) demekti. Ayni adresin iki gorunumu (DMAn_W
 *    yazim icin duz, REG_DMAn okuma ve bos okuma icin volatile) farki
 *    139 -> 4 bayta indirdi. Kural 57'nin SIOMLT_SEND satirinin ayni
 *    sinifi; burada DMA denetimi icin de dogrulandi.
 *
 * 2) `REG_IME = zero = 0;` ZINCIRLI atama. Ayri `zero = 0;` deyimi once
 *    sabiti uretiyor (`movs r5,#0 / ldr r0,=IME`), ROM ise once adresi
 *    yukluyor (`ldr r0,=IME / movs r5,#0 / strh`). Zincirli atamada sabit,
 *    disttaki atamanin SAG tarafi olarak — yani LHS adresi uretildikten
 *    SONRA — olusuyor ve ROM'un sirasi cikiyor. Son 4 bayti bu kapatti.
 *
 * ELENEN YAZIMLAR: `&=` yerine acik oku-ve-yaz (degisiklik yok), bos
 * okumayi degiskene baglamak (degisiklik yok), tum sifirlari duz sabit
 * yazmak (168 bayt / 137 fark — sifir r5'te tutulmuyor, her yazimda
 * yeniden uretiliyor), sifiri tek uzun omurlu yerelde tutup ayri deyimle
 * ilklemek (139 fark).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/shutdown_reset.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define DMA_CLEAR_TIMING 0xC5FF   /* tekrar, gamepak ve zamanlama bitleri */
#define DMA_CLEAR_ENABLE 0x7FFF   /* etkinlestirme biti */
#define RESET_ALL        0xFF

/* Yazim gorunumu volatile DEGIL (kural 57): volatile lvalue yazimdan once
 * olu bir `ldrh` uretiyor. Okuma ve bos okuma volatile kaliyor. */
#define DMA0_W (*(DmaRegs *)0x040000B0)
#define DMA1_W (*(DmaRegs *)0x040000BC)
#define DMA2_W (*(DmaRegs *)0x040000C8)
#define DMA3_W (*(DmaRegs *)0x040000D4)

extern void SoftResetSystem(u32 flags);

/* 0x08065650 */
void ShutdownAndReset(void)
{
    u16 zero;

    REG_IME = zero = 0;

    DMA0_W.control = REG_DMA0.control & DMA_CLEAR_TIMING;
    DMA0_W.control = REG_DMA0.control & DMA_CLEAR_ENABLE;
    REG_DMA0.control;

    DMA1_W.control = REG_DMA1.control & DMA_CLEAR_TIMING;
    DMA1_W.control = REG_DMA1.control & DMA_CLEAR_ENABLE;
    REG_DMA1.control;

    DMA2_W.control = REG_DMA2.control & DMA_CLEAR_TIMING;
    DMA2_W.control = REG_DMA2.control & DMA_CLEAR_ENABLE;
    REG_DMA2.control;

    DMA3_W.control = REG_DMA3H.control & DMA_CLEAR_TIMING;
    DMA3_W.control = REG_DMA3H.control & DMA_CLEAR_ENABLE;
    REG_DMA3H.control;

    REG_BLDCNT   = zero;
    REG_BLDY     = zero;
    REG_IE       = zero;
    REG_DISPSTAT = zero;
    REG_BG0CNT   = zero;
    REG_BG1CNT   = zero;
    REG_BG2CNT   = zero;
    REG_BG3CNT   = zero;

    SoftResetSystem(RESET_ALL);
}
