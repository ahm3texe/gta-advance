/* Bekleyen palet aktarimlarini bosaltma — 0x08013900-0x08013973
 *
 * gRam02022E50 +0x104'teki listeyi yuruyor; bayragi kurulu her dugum icin
 * DMA3 bosalana kadar bekleyip, kesmeler kapaliyken palet transferini
 * baslatiyor ve bayragi temizliyor.
 *
 * Kontrol akisi tools/dump_cfg.py ile cikarildi (8 blok, uc birlesme
 * noktasi) ve etiketlerle yazildi.
 *
 * Sabit kaliplari: +0x104 ofseti `movs r1,#130 / lsls r1,#1` ile,
 * 0x80000000 maskesi `movs r1,#128 / lsls r1,#24` ile KURULUYOR;
 * 0x05000200 ve 0x84000008 HAVUZDAN yukleniyor.
 *
 * Bekleme dongusu: ROM once `cmp r0,#0 / bge` ile bit 31 zaten bossa
 * dongoyu ATLIYOR, aksi halde maskeyle donuyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * DURUM: PARK — 97/120 fark (ROM 116 bayt).  Yapisi dogru, dagitimi degil.
 *
 * KAZANIM: prologun ilk 10 bayti BIREBIR tutuyor.  +0x104 erisiminin
 * dizi degil YAPI UYESI oldugu boylece kanitlandi: `ldr rX,[rY,#260]`
 * kodlanamadigi icin (word yuklemede tavan 124) AGBCC 260'i
 * `movs #130 / lsls #1 / adds` ile kuruyor.  Dizi + sabit ofset yazimi
 * bunu tek havuz sabitine katliyordu.
 *
 * KALAN FARK — dongu degismezi sabitleme.  ROM ucunu de donguden ONCE
 * yazmaca pinliyor:
 *     ldr  r7, =0x04000208      (REG_IME adresi)
 *     movs r3, #0 / mov ip, r3  (sabit 0, YUKSEK yazmacta)
 *     ldr  r5, =0x040000D4      (DMA3 tabani)
 * Bizim surumumuz ucunu de dongu icinde yeniden uretiyor.
 *
 * ELENEN IKI YOL:
 *   1. Dizi + sabit ofset  -> ofset katlandi, 85/116.
 *   2. Bildirilmis extern nesne (`extern vu16 REG_IME;`) yerine
 *      adres-cast makrosu  -> 99/120, DAHA KOTU.  Nesne biciminin
 *      adresi pinleyecegi varsayimi YANLIS cikti.
 *
 * Bu, docs/COMPILER.md'de acik duran "bir fazla canli deger" sinifi.
 * Ayni sinif: step_decay.c, try_engage_target.c, unlink_to_free.c.
 * Genellenebilir bir cozum bulunana kadar park.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/flush_palette_queue.c
 */

#include "gba_types.h"

#define DMA_BUSY    (0x80 << 24)
#define PAL_BASE    0x05000200
#define PAL_STRIDE  5
#define DMA_CONTROL 0x84000008


typedef struct DmaChannel {
    void       *src;            /* +0x00 */
    void       *dst;            /* +0x04 */
    vu32        control;        /* +0x08 */
} DmaChannel;

#define REG_IME (*(vu16 *)0x04000208)
#define DMA3    ((volatile DmaChannel *)0x040000D4)

typedef struct Slot {
    u8           index;         /* +0x00 */
    u8           pad01[2];
    u8           pending;       /* +0x03 */
    void        *src;           /* +0x04 */
    struct Slot *next;          /* +0x08 */
} Slot;

/* +0x104 ofseti `ldr rX,[rY,#imm]` kodlamasina sigmiyor (word yuklemede
   tavan 124), bu yuzden AGBCC 260'i ayri bir yazmaca kuruyor: kalip
   bir dizi aritmetigi degil, YAPI UYESI erisimi. */
typedef struct Root {
    u8    pad000[0x104];
    Slot *queue;                /* +0x104 */
} Root;

extern Root gRam02022E50;

/* 0x08013900 */
void FlushPaletteQueue(void)
{
    Slot *slot;
    Slot *next;
    u16   ime;
    void *src;

    slot = gRam02022E50.queue;
    if (slot == 0)
        return;

loop:
    next = slot->next;
    if (slot->pending == 0)
        goto step;

    src = slot->src;
    if ((s32)DMA3->control < 0) {
        while (DMA3->control & DMA_BUSY)
            ;
    }

    ime = REG_IME;
    REG_IME = 0;
    DMA3->src = src;
    DMA3->dst = (void *)(PAL_BASE + (slot->index << PAL_STRIDE));
    DMA3->control = DMA_CONTROL;
    DMA3->control;
    REG_IME = ime;
    slot->pending = 0;

step:
    slot = next;
    if (slot != 0)
        goto loop;
}
