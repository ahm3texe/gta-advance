/* Donanimi kapatip yumusak sifirlama — 0x08065650-0x080656F3
 *
 * Dort DMA kanalinin denetim alanini iki adimda temizliyor (once
 * tekrar/zamanlama bitleri, sonra etkinlestirme biti), her kanaldan sonra
 * bir defa OKUYOR -- donanimda yazimin oturmasi icin gereken bos okuma,
 * bu yuzden DmaRegs volatile olmali. Ardindan karisim, kesme, ekran ve
 * dort arka plan denetim yazmacini sifirlayip BIOS yumusak sifirlamasini
 * cagiriyor.
 *
 * DURUM: PARK — 180/180 boyut, 139 bayt fark ama 73/89 KOMUT ayni.
 * Yapisi dogru; kalan fark yazmac dagitimi ve iki komutluk siralama.
 *
 * Olculdu (dump_alloc --function ShutdownAndReset): sifir sabiti bizde
 * YEREL dagiticida kisa omurlu pseudo (24: refs 2, omur 4); ROM sifiri
 * r5'te, yani callee-saved bir yazmacta fonksiyon boyunca canli tutuyor.
 * Sifiri tek uzun omurlu yerel degiskene almak farki 158 -> 139 bayta
 * indirdi ve komut eslesmesini 73/89'a cikardi; geri kalani kapatmadi.
 *
 * ELENEN YAZIMLAR: `&=` yerine acik oku-ve-yaz (degisiklik yok), bos
 * okumayi degiskene baglamak (degisiklik yok), REG_IME'yi sabit sifirla
 * yazmak (164 bayta kotulesti).
 *
 * KALAN IKI FARK:
 *   - ROM once adresi yukleyip sonra sifiri uretiyor, biz tersini.
 *   - Her DMA kanalinda bizde bir fazla `ldrh` var.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/shutdown_reset.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define DMA_CLEAR_TIMING 0xC5FF   /* tekrar, gamepak ve zamanlama bitleri */
#define DMA_CLEAR_ENABLE 0x7FFF   /* etkinlestirme biti */
#define RESET_ALL        0xFF

extern void SoftResetSystem(u32 flags);

/* 0x08065650 */
void ShutdownAndReset(void)
{
    u16 zero;

    zero = 0;
    REG_IME = zero;

    REG_DMA0.control = REG_DMA0.control & DMA_CLEAR_TIMING;
    REG_DMA0.control = REG_DMA0.control & DMA_CLEAR_ENABLE;
    REG_DMA0.control;

    REG_DMA1.control = REG_DMA1.control & DMA_CLEAR_TIMING;
    REG_DMA1.control = REG_DMA1.control & DMA_CLEAR_ENABLE;
    REG_DMA1.control;

    REG_DMA2.control = REG_DMA2.control & DMA_CLEAR_TIMING;
    REG_DMA2.control = REG_DMA2.control & DMA_CLEAR_ENABLE;
    REG_DMA2.control;

    REG_DMA3H.control = REG_DMA3H.control & DMA_CLEAR_TIMING;
    REG_DMA3H.control = REG_DMA3H.control & DMA_CLEAR_ENABLE;
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
