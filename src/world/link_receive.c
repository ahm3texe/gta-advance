/* Gelen paketleri toplar — 0x08066798-0x0806686B
 *
 * Alma tamponunu takas edip dort yuvanin her birini siniyor. Bir yuvanin
 * on yarim kelimesinin toplami -13 ise paket saglam sayiliyor: 16 baytlik
 * yuk cagiranin tamponuna kopyalaniyor ve o yuvanin biti sonuc maskesine
 * ekleniyor. Yuva her durumda sifirlaniyor.
 *
 * -13 sabiti kesin: BuildLinkPacket (0x0806673C) saglamayi `~toplam - 12`
 * olarak yaziyor, dolayisiyla saglama dahil toplam her zaman -13 cikar.
 *
 * Takas ve bayrak okuma IME kapaliyken yapiliyor; seri kesme tam o anda
 * tamponu degistirebilir.
 *
 * DURUM: PARK — 204/212 bayt (8 eksik), yapi ve komut dizisi ROM ile ayni.
 *
 * Tek fark yazmac sayisi: ROM UC yuksek yazmac kullaniyor (r8 = global
 * adresi, r9 = hedef tampon, sl = -13 sabiti), biz IKI. Eksik 8 bayt tam
 * olarak o ucuncu yazmacin kaydet/geri-yukle ciftidir. ROM `slot`u da
 * ayrica canli tutup `payload`i sonradan turetiyor; biz ifadeyi yerinde
 * birakinca bile ayni baski olusmuyor.
 *
 * PERMUTER KOSTURULDU (1519 yineleme): temel skor 1095 -> en iyi 195.
 * Ama 195'lik aday SEMANTIK OLARAK YANLIS: `total` degiskenini hem dis
 * dongunun siniri hem ic toplam olarak kullaniyor, ilk yinelemeden sonra
 * sinir bozuluyor. Yalnizca dogru kismi (payload'i `if` icinde yerele
 * almak) alindi, tek basina hicbir sey degistirmedi. Kural: anlasilmamis
 * ya da anlami bozan eslesme kabul edilmez (docs/WORKFLOW.md 6).
 *
 * ELENEN YAZIMLAR: sayaclari s32/u32 (u32 IC DONGUYU duzeltti, 29 -> 8
 * bayt), payload'i degisken/ifade olarak yazmak, if icinde yerele almak.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_receive.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "comm_block.h"

#define SLOT_COUNT      4
#define SLOT_STRIDE     24
#define SLOT_HALFWORDS  10
#define PAYLOAD_BYTES   16
#define CHECKSUM_TOTAL  (-13)
#define CPUSET_COPY_16  0x04000004   /* 32 bit, dort kelime */
#define CPUSET_FILL_16  0x05000004   /* sabit kaynak, 32 bit, dort kelime */

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x08066798 */
u32 ReceiveLinkPackets(u8 *dest)
{
    CommBlock *block;
    void      *swap;
    u8         pending;
    u32        i;
    u32        j;
    s32        total;
    u16       *word;
    u8        *slot;
    s32        zero;

    block = gRam02036338;
    if (block->arrived == 0)
        return 0;

    REG_IME = 0;

    swap            = block->bufEPtr;
    block->bufEPtr  = block->bufDPtr;
    block->bufDPtr  = swap;

    pending        = block->arrived;
    block->arrived = 0;

    REG_IME = 1;

    gRam02036338->byte03 = 0;

    if (pending != 0) {
        for (i = 0; i <= SLOT_COUNT - 1; i++) {
            slot = (u8 *)gRam02036338->bufEPtr + i * SLOT_STRIDE;

            total = 0;
            word = (u16 *)slot;
            for (j = 0; j <= SLOT_HALFWORDS - 1; j++) {
                total += *word;
                word++;
            }

            if ((s16)total == CHECKSUM_TOTAL) {
                CpuSet(slot + 4, dest + i * PAYLOAD_BYTES, CPUSET_COPY_16);
                gRam02036338->byte03 |= 1 << i;
            }

            zero = 0;
            CpuSet(&zero, slot + 4, CPUSET_FILL_16);
        }
    }

    gRam02036338->byte02 |= gRam02036338->byte03;
    return gRam02036338->byte03;
}
