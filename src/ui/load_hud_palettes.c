/* Band B — 0x08030B34 .. 0x08031D23 arasindan dokuz fonksiyon.
 *
 * ONEMLI OLCUM NOTU — `make c-match FILE=src/world/band_b.c` CIKTISI YANILTICI
 * ----------------------------------------------------------------------------
 * tools/agbcc_build.py bir dosyadaki TUM fonksiyonlari PES PESE, en kucuk ROM
 * adresinden baslayarak linkliyor (`base = min(address)`, `SUBALIGN(1)`).  Bu
 * dokuz fonksiyon ROM'da BITISIK DEGIL; aralarinda baska ceviri birimlerine ait
 * fonksiyonlar var.  Sonuc: ilk fonksiyon (SendTextMode1) disindaki her fonksiyon
 * kendi ROM adresinden SABIT bir delta kadar kaymis olarak linkleniyor ve
 * govdesindeki her `bl` o delta kadar yanlis kodlaniyor.  Govde birebir dogru
 * olsa bile `bl` iceren fonksiyon "farkli: 2/N byte" gorunuyor.
 *
 * Bu yuzden her fonksiyon AYRICA tek fonksiyonluk bir dosyada (base = kendi ROM
 * adresi, `bl` dogru) olculdu.  Tek fonksiyonluk olcumler — dogru olanlar:
 *     SendTextMode1  12  BYTE-MATCHING
 *     LoadHudPalettes 124  BYTE-MATCHING
 *     TriggerEvent39  12  BYTE-MATCHING
 *     GetRecordNodeById  14  BYTE-MATCHING
 *     ScaleMagnitude 132  BYTE-MATCHING
 *     ClearHudRowsAB  72  BYTE-MATCHING
 *     ReleaseActorAndSlot  64  BYTE-MATCHING
 *     BlitStripClipLeft4bpp 470  eslesmedi (asagida ayrintili)
 *     PushSlotQueueEntry 140  RAM SEMBOLU EKSIK (asagida ayrintili)
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_b.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* ---- 0x08030EAC — 124 bayt, BYTE-MATCHING -------------------------------
 *
 * Iki palet blogunu kuruyor.  Once 0x02026BE0'daki bayragi sifirliyor, sonra
 * kesmeleri kapatip DMA3 ile yigindaki 0x3333 sabitini 0x0600A780'e 16 yarim
 * soz olarak dolduruyor (kaynak sabit adresli: 0x81000010).  Kesmeler geri
 * acilip CpuSet ile 0x08347C28 -> 0x0600A7A0 kopyalaniyor, ardindan ikinci bir
 * DMA3 aktarimi 0x08348B28 -> 0x0600A7C0 (32 yarim soz) yapiliyor.
 *
 * OLCULEN AYRINTILAR:
 *  - `fill` YIGINDA ve `volatile` olmali (kural 3).  volatile olmadan agbcc
 *    adres alma ile sabit yuklemeyi yeniden siraliyor.
 *  - Sabit DOGRUDAN store ifadesinde: `ldr r3,=0x3333 / adds r0,r3,#0` cifti
 *    (src/ui/hud_fields.c MEKANIZMA 1) ancak HImode store sabitinden cikiyor.
 *  - `REG_DMA3.control;` satirlari olu okuma degil: ROM `ldr r0,[r4,#8]` ile
 *    denetim kelimesini geri okuyor (kural 62, volatile gorunum).
 *  - 0x02026BE0 icin ram_map kaydi YOK; ROM ofsetsiz (`strb r0,[r1,#0]`)
 *    kullandigi icin kural 1 devreye girmiyor ve cast yazimi ayni baytlari
 *    veriyor.  Sembol eklenirse `extern u8 gRam02026BE0;` yazimi tercih edilir.
 */

#define PAL_FLAG      (*(u8 *)0x02026BE0)
#define PAL_FILL      0x3333
#define PAL_FILL_DST  ((void *)0x0600A780)
#define PAL_SRC       ((const void *)0x08347C28)
#define PAL_DST       ((void *)0x0600A7A0)
#define PAL_SRC2      ((const void *)0x08348B28)
#define PAL_DST2      ((void *)0x0600A7C0)
#define DMA_FILL16    0x81000010        /* enable, kaynak sabit, 16 yarim soz */
#define DMA_COPY16    0x80000020        /* enable, 32 yarim soz */
#define CPUSET_COPY   0x30

extern void CpuSet(const void *src, void *dst, u32 control);

/* 0x08030EAC */
void LoadHudPalettes(void)
{
    vu16 fill;
    u16 ime;

    PAL_FLAG = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = PAL_FILL;
    REG_DMA3.src = (const void *)&fill;
    REG_DMA3.dst = PAL_FILL_DST;
    REG_DMA3.control = DMA_FILL16;
    REG_DMA3.control;
    REG_IME = ime;

    CpuSet(PAL_SRC, PAL_DST, CPUSET_COPY);

    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = PAL_SRC2;
    REG_DMA3.dst = PAL_DST2;
    REG_DMA3.control = DMA_COPY16;
    REG_DMA3.control;
    REG_IME = ime;
}

