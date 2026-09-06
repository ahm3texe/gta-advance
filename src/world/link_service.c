/* Baglanti kare servisi — 0x0806686C-0x08066903
 *
 * Blok hazirsa gonderme/alma tamponlarini takas edip yeni bir cok
 * oyunculu aktarim baslatiyor: SIOCNT'nin hata bitini kaydediyor,
 * gonderilecek kelimeyi yaziyor, aktarimi baslatiyor ve zaman asimi icin
 * Timer3'u kesmeyle birlikte aciyor.
 *
 * Blok hazir degilse dort kareye kadar bekliyor; dorduncuden sonra BIOS
 * kesme bayragini kaldirip pes ediyor. Bu nadir yol ROM'da fonksiyonun
 * SONUNDA duruyor (kural 49), o yuzden kaynakta da `else` dali.
 *
 * SIOCNT burada 32 BIT okunuyor: hata biti ust yarim kelimeyle birlikte
 * tek `ldr` ile aliniyor, sonra 25 sola + 31 saga kaydirmayla ayikliyor.
 *
 * DURUM: PARK — 144/152 bayt. Komut dizisi ROM ile ayni; kalan fark
 * ROM'un global adresini r4'te (callee-saved) tutmasi, bizim r3'te
 * tutmamiz. Eksik 8 bayt o push/pop cifti ve hizalamasi.
 *
 * SIO_PORT (volatile OLMAYAN gorunum) DOGRU bicim: ROM'da `send`
 * yaziminin oncesinde okuma YOK, gba_io.h'deki volatile REG_SIO ise olu
 * bir `ldrh` uretiyor. Bu bicim 0x08066904'te olculdu.
 *
 * ELENEN: bios bayragini `*(volatile u16 *)&gBiosIrqFlags` ile yazmak
 * (degisiklik yok), denetimi SIO_PORT.control ile oku-yaz (degisiklik
 * yok), hata bitini 32 bit yerine control uzerinden almak (148 bayt, ama
 * ROM 32 bit `ldr` yapiyor -- bicim yanlis).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_service.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

/* SIOCNT + SIOMLT_SEND'in volatile OLMAYAN gorunumu. gba_io.h'deki
 * volatile SioRegs uzerinden `send` yazimi agbcc'ye olu bir `ldrh`
 * urettiriyor; volatile olmayan gorunum yalniz `strh` birakiyor.
 * 0x08066904'te olculdu. */
typedef struct SioPort {
    u16 control;                /* +0x00 */
    u16 send;                   /* +0x02 */
} SioPort;

#define SIO_PORT (*(SioPort *)REG_SIOCNT_ADDR)

#define RETRY_LIMIT     3
#define SIO_ERROR_SHIFT 25          /* bit 6'yi 32 bitin tepesine tasir */
#define SIO_START       0x0080
#define SIO_SEND_IDLE   0xFEFE
#define TM3_ON_WITH_IRQ 0x00C0
#define BIOS_IRQ_SERIAL 0x0080

/* 0x0806686C */
void ServiceLinkFrame(void)
{
    CommBlock  *block;
    LinkPacket *swapPacket;
    void       *swapBuf;

    block = gRam02036338;

    if (block->byte0 != 0) {
        if (block->ready != 0 && block->byte01 != 0 && block->ready06 != 0) {
            block->recvLen = -1;

            swapBuf         = block->bufDPtr;
            block->bufDPtr  = block->bufCPtr;
            block->bufCPtr  = swapBuf;

            swapPacket       = (LinkPacket *)block->bufBPtr;
            block->bufBPtr   = block->packetPtr;
            block->packetPtr = swapPacket;

            block->ready = 0;

            gRam02036338->sendLen  = 0;
            gRam02036338->errorBit =
                (REG_SIOCNT32 << SIO_ERROR_SHIFT) >> 31;

            SIO_PORT.send   = SIO_SEND_IDLE;
            REG_SIOCNT = REG_SIOCNT | SIO_START;
            REG_TM3CNT_H    = TM3_ON_WITH_IRQ;
        }
    } else {
        if (block->retry <= RETRY_LIMIT) {
            block->retry++;
        } else {
            REG_IME = 0;
            gBiosIrqFlags = gBiosIrqFlags | BIOS_IRQ_SERIAL;
            REG_IME = 1;
        }
    }
}
