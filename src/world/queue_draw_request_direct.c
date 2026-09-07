/* Cizim istegini dogrudan kuyruga alma — 0x08012E18-0x08012E77
 *
 * QueueDrawRequest (src/world/queue_draw_request.c) ile ayni kuyruk yazimi;
 * fark: havuz cagrilari yok, istegin +0x00 baglantisi ve onun hedefi
 * sifir degilse dogrudan giris yaziliyor. Donus degeri yok.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/queue_draw_request_direct.c
 */

#include "gba_types.h"
#include "gba_io.h"
#define KIND_MAX   8
#define QUEUE_MAX  127
typedef struct Req { u32 *link; u32 handle; u16 id; u8 kind; u8 pad0b; u32 payload; } Req;
typedef struct Slot { u32 payload; u32 handle; u16 id; u16 tag; } Slot;
extern u32  gRam02022ABC;
extern Slot gRam0201ECB0[];
void QueueDrawRequestDirect(Req *req, u32 tag)
{
    u32 count; u8 *base; u8 *altBase; Slot *slot; u32 scaled;
    if (req->link == 0) return;
    if (*req->link == 0) return;
    if (req->kind > KIND_MAX) return;
    count = gRam02022ABC;
    if (count > QUEUE_MAX) return;
    REG_IME = 0;
    base = (u8 *)gRam0201ECB0;
    scaled = count * sizeof(Slot);
    slot = (Slot *)(base + scaled);
    slot->payload = req->payload;
    altBase = base + 4;
    *(u32 *)(altBase + scaled) = req->handle;
    slot->tag = tag;
    slot->id = req->id;
    gRam02022ABC = count + 1;
    REG_IME = 1;
}
