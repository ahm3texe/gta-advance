/* Yuva kuyruguna giris ekleme — 0x08031C98-0x08031D23
 *
 * gFlagsB[slot] o yuvadaki giris sayisi; sayi 3'u gectiyse once
 * FUN_0802ebdc ile bosaltiliyor ve sayi yeniden okunuyor. Sonra alti
 * paralel tabloya [sayi][slot] konumundan yaziliyor (besi u16, biri u32)
 * ve sayac bir artiriliyor.
 *
 * Tablolar SUTUN duzeninde: satir adimi 8 giris, yani u16 tablolarda 16,
 * u32 tabloda 32 bayt. Satir sayisi (N) BILINMIYOR; bu yuzden semboller
 * ilk boyutu acik birakilmis dizi olarak bildiriliyor.
 *
 * KURAL 1 BURADA OLCULDU: tabanlar `((u16 (*)[8])0x02026BA0)` gibi cast
 * ile yazilirsa agbcc havuz yuklemesini adres hesabinin SONUNA koyuyor
 * (ROM ONUNE koyuyor); dusen yazmac baskisi `n`i callee-saved bir
 * yazmaca tasimiyor ve bir yazmac daha az saklaniyor -> 132 bayt (ROM
 * 140), 25/69 komut. Extern DIZI sembolleriyle fark sifir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/push_slot_queue_entry.c
 */

#include "gba_types.h"

#define QUEUE_MAX 3
#define SLOTS     8

extern u8  gFlagsB[];               /* 0x02026EF0 — yuva basina giris sayaci */
extern u16 gRam02026BA0[][SLOTS];
extern u16 gRam02027230[][SLOTS];
extern u32 gRam02026C10[][SLOTS];
extern u16 gRam02026F80[][SLOTS];
extern u16 gRam02026C90[][SLOTS];
extern u16 gRam02026F40[][SLOTS];

extern void FUN_0802ebdc(u32 slot);

/* 0x08031C98 */
void PushSlotQueueEntry(u32 slot, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f)
{
    u8 *countp;
    s32 n;

    countp = &gFlagsB[slot];
    n = *countp;
    if (n > QUEUE_MAX) {
        FUN_0802ebdc(slot);
        n = *countp;
    }

    gRam02026BA0[n][slot] = a;
    gRam02027230[n][slot] = b;
    gRam02026C10[n][slot] = c;
    gRam02026F80[n][slot] = d;
    gRam02026C90[n][slot] = e;
    gRam02026F40[n][slot] = f;

    *countp = *countp + 1;
}
