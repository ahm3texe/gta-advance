#ifndef GUARD_GBA_IO_H
#define GUARD_GBA_IO_H

#include "gba_types.h"

/* Donanim yazmaclari.
 *
 * DIKKAT: `volatile` burada semantik degil, agbcc'de komut siralama
 * dugmesidir (docs/COMPILER.md kural 4 ve 12). Asagidaki nitelemeler
 * OLCULEREK secildi; degistirmek eslesmeyi bozar. Yeni bir yazmac
 * eklerken iki yonu de dene ve `make matching` ile dogrula. */

typedef struct {
    const void *src;
    void *dst;
    u32 control;
} DmaChannel;

/* Ham adresler: bazi yerlerde makro degil SABIT IFADE gerekiyor
 * (docs/COMPILER.md kural 22 -- isaretcinin sabitten yeniden atanmasi). */
#define REG_DISPCNT_ADDR  0x04000000
#define REG_DISPSTAT_ADDR 0x04000004
#define REG_VCOUNT_ADDR   0x04000006
#define REG_BG1VOFS_ADDR  0x04000016
#define REG_BLDCNT_ADDR   0x04000050
#define REG_BLDALPHA_ADDR 0x04000052
#define REG_BLDY_ADDR     0x04000054
#define REG_DMA3_ADDR     0x040000D4
#define REG_TM3CNT_ADDR   0x0400010C
#define REG_SIOCNT_ADDR   0x04000128
#define REG_RCNT_ADDR     0x04000134
#define REG_SIOMLT_SEND_ADDR 0x0400012A
#define REG_TM3CNT_H_ADDR 0x0400010E
#define REG_IE_ADDR       0x04000200
#define REG_IF_ADDR       0x04000202
#define REG_WAITCNT_ADDR  0x04000204
#define REG_IME_ADDR      0x04000208

#define REG_DISPCNT  (*(u16 *)REG_DISPCNT_ADDR)
#define REG_DISPSTAT (*(volatile u16 *)REG_DISPSTAT_ADDR)
#define REG_VCOUNT   (*(volatile u16 *)REG_VCOUNT_ADDR)
#define REG_BG1VOFS  (*(volatile u16 *)REG_BG1VOFS_ADDR)
#define REG_BLDCNT   (*(u16 *)REG_BLDCNT_ADDR)
#define REG_BLDALPHA (*(u16 *)REG_BLDALPHA_ADDR)
#define REG_BLDY     (*(u16 *)REG_BLDY_ADDR)
#define REG_DMA3     (*(volatile DmaChannel *)REG_DMA3_ADDR)
/* TM3CNT tek 32-bit yazimla hem yeniden yukleme hem denetim alanini siliyor. */
#define REG_TM3CNT   (*(u32 *)REG_TM3CNT_ADDR)
#define REG_SIOCNT   (*(volatile u16 *)REG_SIOCNT_ADDR)
#define REG_RCNT     (*(volatile u16 *)REG_RCNT_ADDR)
#define REG_SIOMLT_SEND (*(volatile u16 *)REG_SIOMLT_SEND_ADDR)
/* SIO denetimi ve gonderme kelimesi ard arda; ROM ikisini TEK taban
 * yazmaciyla yaziyor (`strh r0,[r2,#2]`), ayri mutlak adresler
 * fazladan literal uretiyor. Bu yuzden struct gorunumu gerekli. */
typedef struct SioRegs {
    u16 control;                    /* +0x00 */
    u16 send;                       /* +0x02 */
} SioRegs;
#define REG_SIO (*(volatile SioRegs *)REG_SIOCNT_ADDR)
#define REG_TM3CNT_H (*(volatile u16 *)REG_TM3CNT_H_ADDR)
/* SIOCNT bazen 32 bit okunuyor: hata biti ust yarim kelimeyle birlikte
 * tek `ldr` ile aliniyor (0x0806686C'de olculdu). */
#define REG_SIOCNT32 (*(volatile u32 *)REG_SIOCNT_ADDR)
#define REG_IE       (*(volatile u16 *)REG_IE_ADDR)
#define REG_IF       (*(volatile u16 *)REG_IF_ADDR)
#define REG_WAITCNT  (*(volatile u16 *)REG_WAITCNT_ADDR)
#define REG_IME      (*(volatile u16 *)REG_IME_ADDR)

/* BIOS kesme denetim bayraklari. volatile DEGIL: eklenince agbcc sabiti
 * bellekten once yukluyor ve ROM'dan sapiyor. */
extern u16 gBiosIrqFlags;   /* 0x03007FF8 */

/* DMA kanallari, denetim alani u16 olarak gorulen bicim.
 *
 * gba_io.h'deki DmaChannel denetimi tek u32 olarak yaziyor (DMA3 doldurma
 * icin dogru bicim). Kanal kapatma yolu ise YALNIZ ust yarim kelimeyi
 * maskeliyor, o yuzden ayri bir gorunum gerekiyor. ROM taban olarak
 * kaynagi (SAD) tutup +10 ofsetiyle yaziyor; struct bu yerlesimi veriyor. */
typedef struct DmaRegs {
    const void *src;                /* +0x00 */
    void       *dst;                /* +0x04 */
    u16         count;              /* +0x08 */
    u16         control;            /* +0x0A */
} DmaRegs;

#define REG_DMA0 (*(volatile DmaRegs *)0x040000B0)
#define REG_DMA1 (*(volatile DmaRegs *)0x040000BC)
#define REG_DMA2 (*(volatile DmaRegs *)0x040000C8)
#define REG_DMA3H (*(volatile DmaRegs *)0x040000D4)

#define REG_BG0CNT   (*(u16 *)0x04000008)
#define REG_BG1CNT   (*(u16 *)0x0400000A)
#define REG_BG2CNT   (*(u16 *)0x0400000C)
#define REG_BG3CNT   (*(u16 *)0x0400000E)

#define EWRAM_BASE 0x02000000
#define IWRAM_BASE 0x03000000
#define VRAM_BASE  0x06000000
#define OAM_BASE   0x07000000

#endif /* GUARD_GBA_IO_H */
