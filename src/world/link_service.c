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
 * tutmamiz. Eksik 8 bayt o push/pop cifti ve hizalamasi:
 *   ROM `push {r4,lr}` (2) + `pop {r4} / pop {r0} / bx r0` (6) = 8
 *   biz  push YOK        (0) + `bx lr`                        (2) = 2
 *   ustune ROM'un komut govdesi 2 bayt uzun oldugu icin ilk havuz
 *   4 hizasina 2 bayt dolgu aliyor.  59 komutluk govde birebir ayni.
 *
 * SIO_PORT (volatile OLMAYAN gorunum) DOGRU bicim: ROM'da `send`
 * yaziminin oncesinde okuma YOK, gba_io.h'deki volatile REG_SIO ise olu
 * bir `ldrh` uretiyor. Bu bicim 0x08066904'te olculdu.
 *
 * KURAL 57 BURADA DA GECERLI — ONCEKI "degisiklik yok" NOTU YANLISTI
 * (duzeltildi 2026-09-07): `gBiosIrqFlags`i `*(volatile u16 *)&...` ile
 * okuyup yazmak BOYUTU degistirmiyor (144) ama `else` dalini ROM'un
 * bicimine tam oturtuyor.  Volatile olmayan yazim sabiti once yukluyor
 * (`ldr r1,=flags / mov r0,#0x80 / ldrh r3,[r1] / orr r0,r3 / strh r0,[r1]`);
 * volatile gorunum ROM'un sirasini veriyor
 * (`ldr r2,=flags / ldrh r0,[r2] / mov r1,#0x80 / orr r0,r1 / strh r0,[r2]`)
 * ve REG_IME adresini de ROM gibi r3'e itiyor.  Yani olcut yalniz bayt
 * sayisi degil, komut dizisi olmali.  Bu haliyle ROM'dan SAPAN tek yer
 * prolog/epilog ve `block`/takas gecicisinin yazmaclari kaldi.
 *
 * DAGITIM TESHISI (dump_alloc): global adres pseudo'su (p25, refs 3 /
 * omur 70 / oncelik 0.043) global dagiticinin SON allocno'su; r0-r3
 * bosta oldugu icin r3'u aliyor.  ROM'da r4'e dusmesi icin dordunun de
 * cakismasi gerekiyor.  Kilit nokta L4 blogundaki TAKAS GECICISI: ROM onu
 * r3'e, biz r1'e koyuyoruz.  Geciciyi r3'e itebilen her yazim ayni anda
 * `block`u da r2'den r3'e kaydiriyor, o yuzden hicbiri tam oturmuyor.
 *
 * ELENEN: denetimi SIO_PORT.control ile oku-yaz (degisiklik yok), hata
 * bitini 32 bit yerine control uzerinden almak (148 bayt, ama ROM 32 bit
 * `ldr` yapiyor -- bicim yanlis).
 *
 * ELENEN YAZIMLAR (2026-09-07, hepsi 144 bayt / push YOK): ikinci bir
 * `CommBlock *live` yereli (blok icinde ve blogun basinda); `u8 flag` ile
 * byte0'i yerele almak (kural 55); `u8 retry` yereli; disardaki testi
 * `gRam02036338->byte0` ile yazmak; `SioPort *sio` yereli; errorBit'i ara
 * degiskene almak; `&&` yerine ic ice `if`; else dalini global uzerinden
 * yazmak; uc ayri bolge isaretcisi (kural 59); takasi acik iki adima
 * bolmek; iki geciciyi tek `void *` yapmak (block r1'e kayiyor); ikisini
 * de `LinkPacket *` / `void *` yapmak; paket takasini once yapmak;
 * `recvLen = -1`i takaslardan sonraya almak; TM3'u SIO_START'tan once
 * yazmak; bildirim sirasinin alti permutasyonu; `block` yerelini tamamen
 * kaldirip her seyi `gRam02036338->` ile yazmak.
 *
 * IKI YAZIM 152 BAYTA ULASIYOR ama komut SIRASI ROM'a uymuyor, o yuzden
 * ALINMADI: (1) iki takasin okumalarini one alip yazmalarini arkaya
 * toplamak (`push {r4,lr}` cikiyor, 19/152 bayt fark) ve (2) `ready = 0`i
 * takaslardan once yazmak (39/152).  Ikisi de baskiyi bir artirip adresi
 * r4'e itiyor ama `block`u r2'den r3'e kaydiriyor; ROM'da `block` r2'de
 * ve takas gecicisi r3'te.  Yani eksik olan sey "bir fazla canli deger"
 * degil, gecicinin r3'e, `block`un r2'de kalmasi.
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
            *(volatile u16 *)&gBiosIrqFlags =
                *(volatile u16 *)&gBiosIrqFlags | BIOS_IRQ_SERIAL;
            REG_IME = 1;
        }
    }
}
