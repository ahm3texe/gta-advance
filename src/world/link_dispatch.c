/* Seri (SIO) kesme isleyicisi — 0x08066904-0x08066A3F
 *
 * Cok oyunculu aktarim bittiginde cagriliyor. Sirasiyla:
 *   - blok etkinse (byte0 == 1) SIOCNT'nin baslatma bitini indiriyor,
 *   - dort SIOMULTI yazmacini (0x04000120) sekiz baytlik tek bir kopyayla
 *     yigina aliyor,
 *   - SIOCNT'yi 32 bit okuyup hata bitini (bit 6) errorBit'e yaziyor,
 *   - 0. yuvadan 0xFEFE (bos/idle kelime) geldiyse ve alma sayaci
 *     tamamlandiysa kareyi kapatiyor: alma sayacini -1'e cekiyor,
 *     alma tamponlarini takas ediyor, gonderme bitmisse gonderme
 *     tamponlarini da takas edip sayaci sifirliyor ve IME kapaliyken
 *     BIOS kesme bayragina seri bitini ekliyor,
 *   - gonderme sayaci 9'u gecmediyse siradaki yarim kelimeyi
 *     SIOMLT_SEND'e koyup sayaci ilerletiyor,
 *   - alma sayaci negatif degilse dort yuvanin her birine (24 bayt
 *     adimla) o karenin kelimesini yaziyor; sayac 9 ise paketi hazir
 *     isaretliyor, sonra sayaci ilerletiyor,
 *   - blok etkinse Timer3'u durduruyor; gonderme sayaci 10'u gecmediyse
 *     baslatma bitini ve Timer3'u kesmeyle birlikte yeniden aciyor,
 *   - son olarak yeniden deneme sayacini sifirliyor.
 *
 * OLCULEN UC AYRINTI (hepsi eslesmeyi belirliyor):
 *
 * 1) SIOMULTI kopyasi 4 hizali OLMALI. `u16 data[4]` tek basina 2 hizali
 *    bir yapi uretiyor ve agbcc `memcpy` cagiriyor; `u32 word[2]` ile
 *    birlik yapinca ROM'daki `ldr [r0,#4] / ldr [r0,#0] / str / str`
 *    ciftine dusuyor.
 *
 * 2) SIOMLT_SEND yazimi VOLATILE OLMAYAN yapi gorunumu istiyor.
 *    gba_io.h'deki `REG_SIO.send = x` (volatile SioRegs) agbcc'de
 *    yazimdan once gereksiz bir `ldrh [r2,#2]` uretiyor; ayni adresin
 *    volatile olmayan gorunumu yalniz `strh` biraktiriyor.
 *
 * 3) Ilk SIOCNT okumasi VOLATILE OLMAMALI, BIOS bayragi ise VOLATILE
 *    OLMALI — ikisi de olculdu:
 *      - `cnt = REG_SIOCNT` (volatile) okumayi yerinde tutuyor ama
 *        `ldrh r0 / adds r2,r0,#0` cifti uretiyor (+2 komut, hizalamayla
 *        +4 bayt). Volatile olmayan `SIO_PORT.control` okumasi
 *        fonksiyon basina cekiliyor ve ROM'daki `ldrh r2,[r3]` cikiyor.
 *      - `gBiosIrqFlags | 0x80` (volatile degil) sabiti bellekten ONCE
 *        yukluyor ve o blogun tamamini baska yazmaclara dagitiyor;
 *        `*(vu16 *)&gBiosIrqFlags` ile ROM'daki `ldrh r0 / movs r1,#128
 *        / orrs r0,r1` sirasi ve sifir sabitinin r4'te durmasi geri
 *        geliyor. Tek fark buydu: 13 komut, 0 bayt boy farki.
 *
 * ELENEN YAZIMLAR (hepsi olculdu, hicbiri fark yaratmadi):
 *   cnt tipini u16/u32/s32 yapmak, bildirimini one almak, ic ice `if`,
 *   tanimda ilklemek; `gBiosIrqFlags |= ...`, `0x80 | gBiosIrqFlags`,
 *   once yerele almak, `(u16)` ile daraltmak. Sabiti `+` ile eklemek ve
 *   IME'yi volatile olmayan yazmak eslesmeyi BOZUYOR.
 *
 * NOT (baska bir dosya, burada degistirilmedi): src/world/link_service.c
 * ayni iki tuzaga dusuyor — `REG_SIO.send` fazladan `ldrh` uretiyor ve
 * `gBiosIrqFlags | 0x80` volatile olmadigi icin o fonksiyon 148/152
 * baytta kaliyor. Yukaridaki iki yazim oraya da uygulanabilir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_dispatch.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

#define SIO_START        0x0080
#define SIO_START_CLEAR  0xFF7F
#define SIO_SEND_IDLE    0xFEFE
#define SIO_ERROR_SHIFT  25         /* bit 6'yi 32 bitin tepesine tasir */
#define TM3_ON_WITH_IRQ  0x00C0
#define BIOS_IRQ_SERIAL  0x0080
#define SLOT_COUNT       4
#define SLOT_STRIDE      24
#define SEND_LAST        9          /* tampondaki son yarim kelimenin indisi */
#define FRAME_DONE       10         /* sayaclarin ust siniri */

/* Dort SIOMULTI yazmaci tek blok olarak okunuyor. Birligin u32 uyesi
 * hizalamayi 4'e cikariyor; onsuz agbcc kopyayi memcpy'ye ceviriyor. */
typedef union SioMulti {
    u32 word[2];
    u16 data[SLOT_COUNT];
} SioMulti;

#define REG_SIOMULTI (*(volatile SioMulti *)0x04000120)

/* SIOCNT + SIOMLT_SEND'in volatile OLMAYAN gorunumu: ilk denetim
 * okumasi ve gonderme yazimi bunu kullaniyor (bkz. bas yorum). */
typedef struct SioPort {
    u16 control;                    /* +0x00 */
    u16 send;                       /* +0x02 */
} SioPort;

#define SIO_PORT  (*(SioPort *)REG_SIOCNT_ADDR)
#define BIOS_IF   (*(vu16 *)&gBiosIrqFlags)

/* 0x08066904 */
void SerialIrqHandler(void)
{
    SioMulti recv;
    void    *swapBuf;
    s32      i;
    u16      cnt;

    cnt = SIO_PORT.control;
    if (gRam02036338->byte0 == 1 && (cnt & SIO_START) != 0)
        REG_SIOCNT = REG_SIOCNT & SIO_START_CLEAR;

    recv = REG_SIOMULTI;

    gRam02036338->errorBit = (REG_SIOCNT32 << SIO_ERROR_SHIFT) >> 31;

    if (recv.data[0] == SIO_SEND_IDLE && gRam02036338->recvLen > SEND_LAST) {
        gRam02036338->recvLen = -1;

        swapBuf               = gRam02036338->bufDPtr;
        gRam02036338->bufDPtr = gRam02036338->bufCPtr;
        gRam02036338->bufCPtr = swapBuf;

        if (gRam02036338->ready != 0) {
            swapBuf                 = gRam02036338->bufBPtr;
            gRam02036338->bufBPtr   = gRam02036338->packetPtr;
            gRam02036338->packetPtr = swapBuf;
            gRam02036338->ready     = 0;
            gRam02036338->sendLen   = 0;
        }

        REG_IME = 0;
        BIOS_IF = BIOS_IF | BIOS_IRQ_SERIAL;
        REG_IME = 1;
    }

    if (gRam02036338->sendLen <= SEND_LAST)
        SIO_PORT.send = *(u16 *)(gRam02036338->sendLen * 2 +
                                 (u8 *)gRam02036338->bufBPtr);

    if (gRam02036338->sendLen <= FRAME_DONE)
        gRam02036338->sendLen = gRam02036338->sendLen + 1;

    if (gRam02036338->recvLen >= 0) {
        for (i = 0; i < SLOT_COUNT; i++) {
            *(u16 *)(gRam02036338->recvLen * 2 +
                     ((u8 *)gRam02036338->bufCPtr + i * SLOT_STRIDE)) =
                recv.data[i];
        }
        if (gRam02036338->recvLen == SEND_LAST)
            gRam02036338->arrived = 1;
    }

    if (gRam02036338->recvLen <= FRAME_DONE)
        gRam02036338->recvLen = gRam02036338->recvLen + 1;

    if (gRam02036338->byte0 != 0)
        REG_TM3CNT_H = 0;

    if (gRam02036338->sendLen <= FRAME_DONE && gRam02036338->byte0 != 0) {
        REG_SIOCNT = REG_SIOCNT | SIO_START;
        REG_TM3CNT_H = TM3_ON_WITH_IRQ;
    }

    gRam02036338->retry = 0;
}
