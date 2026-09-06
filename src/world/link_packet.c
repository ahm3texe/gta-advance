/* Gonderilecek paketi kurar — 0x0806673C-0x08066797
 *
 * Basliga kimlik baytini ve iki alanin XOR'unu yaziyor, sağlama alanini
 * sifirliyor, 16 baytlik yuku CpuSet ile kopyaliyor, sonra paketin on
 * yarim kelimesini toplayip sağlamayi `~toplam - 12` olarak yaziyor ve
 * "hazir" bayragini kaldiriyor.
 *
 * Sağlama alani toplama DAHIL: once sifirlandigi icin sonuc etkilenmiyor.
 *
 * Global her erisimde yeniden okunuyor cunku yazimlar paketin isaretcisi
 * uzerinden gidiyor ve isaretcinin kendisiyle ortusebilir; agbcc bu
 * yuzden CSE yapmiyor.
 *
 * DURUM: PARK — 92/92 boyut, 20 bayt fark, tek komutluk sapma.
 *
 * Kalan tek fark: ROM dongu sonrasi `gRam02036338` adresini r4'te
 * TUTUYOR (`ldr r2, [r4, #0]`), bizim derleme onu havuzdan yeniden
 * yukluyor (`ldr r0, [pc]; ldr r2, [r0, #0]`).
 *
 * Olculdu (dump_alloc --function BuildLinkPacket): agbcc ayni adres
 * sabiti icin IKI ayri pseudo uretiyor --
 *     27  mem[.LC0]  refs 5  omur 46  oncelik 0,217  -> r4
 *     74  mem[.LC0]  refs 2  omur  4  oncelik 0,500  -> r0
 * ROM'da tek canli aralik var. CSE dongu blogunu asmadigi icin ikinci
 * pseudo doguyor; bu, kural 50'nin belgelenmis sinirinin tersi yonu ve
 * kaynak tarafinda BIRLESTIRME kolu yok.
 *
 * ELENEN YAZIMLAR (hepsi ayni 20 baytta kaldi):
 *   - sayaci s32/u32, do-while/while/for, indeksli erisim  (5 yazim)
 *   - kuyruk blogunu yerel `CommBlock *`e almak
 *   - iki kuyruk atamasinin sirasini degistirmek (36 bayta kotulesti)
 *   - `total` turunu u32 yapmak
 * Sayaci u32 yapmak 29 -> 20 bayta indirdi; digerleri hicbir sey
 * degistirmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_packet.c
 */

#include "gba_types.h"
#include "comm_block.h"

#define PACKET_HALFWORDS  10
#define CHECKSUM_BIAS     12
#define CPUSET_COPY_16    0x04000004   /* 32 bit, dort kelime */

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x0806673C */
void BuildLinkPacket(const void *payload)
{
    u32  i;
    s32  total;
    u16 *word;

    total = 0;

    gRam02036338->packetPtr->ident    = gRam02036338->ident;
    gRam02036338->packetPtr->mix      = gRam02036338->byte02 ^ gRam02036338->byte03;
    gRam02036338->packetPtr->checksum = 0;

    CpuSet(payload, gRam02036338->packetPtr->payload, CPUSET_COPY_16);

    i = 0;
    word = (u16 *)gRam02036338->packetPtr;
    do {
        total += *word;
        word++;
        i++;
    } while (i <= PACKET_HALFWORDS - 1);

    gRam02036338->packetPtr->checksum = ~total - CHECKSUM_BIAS;
    gRam02036338->ready = 1;
}
