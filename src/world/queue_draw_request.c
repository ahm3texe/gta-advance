/* Cizim istegini kuyruga alma — 0x08012F88-0x08013097
 *
 * IKI FONKSIYON, AYNI GOVDE.  tools/find_twins.py %100 benzerlik verdi;
 * ROM govdeleri komut komut ayni, yalnizca havuz adresleri ve dal
 * hedefleri kayik -- ayni kaynak iki kez derlenmis (docs/WORKFLOW.md §10).
 *
 * Istek once FUN_080089b0'a veriliyor; sifir donmezse is bitti sayilip 1
 * doner.  Sonra FUN_08008a28'den bir tutamac aliniyor; tutamac 0 ise 0
 * doner.  Tur 8'i asiyorsa ya da kuyruk 128 girisi doldurmussa yine 1.
 * Aksi halde IME kapatilip 12 bayt adimli kuyruga bir giris yazilip
 * sayac artiriliyor ve IME geri aciliyor.
 *
 * IKI OLCUM:
 *   - `tag` parametresi u16 DEGIL, tam soz.  u16 yazilirsa girise
 *     ROM'da olmayan bir `lsls #16 / lsrs #16` sifir genisletmesi
 *     ekleniyor; daralma zaten sondaki `strh`de oluyor.
 *   - +0x04 alani AYRI BIR TABAN yerelinden yazilmali (release_slot.c
 *     ile ayni olcut): ROM bir kez hesaplanan olcegi iki tabana ekliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/queue_draw_request.c
 */

#include "gba_types.h"
#include "gba_io.h"

#define KIND_MAX   8
#define QUEUE_MAX  127

typedef struct Req {
    u8   pad00[4];
    u32  handle;                /* +0x04 */
    u16  id;                    /* +0x08 */
    u8   kind;                  /* +0x0A */
    u8   pad0b;
    u32  payload;               /* +0x0C */
} Req;

typedef struct Slot {
    u32 payload;                /* +0x00 */
    u32 handle;                 /* +0x04 */
    u16 id;                     /* +0x08 */
    u16 tag;                    /* +0x0A */
} Slot;

typedef struct Pool { u8 pad00[4]; } Pool;

extern Pool gRam0201D6D0;
extern u32  gRam02022ABC;
extern Slot gRam0201ECB0[];

extern s32 FUN_080089b0(Pool *pool, Req *req);
extern u32 FUN_08008a28(Pool *pool, u16 id, Req *req);

/* 0x08012F88 */
u32 QueueDrawRequest(Req *req, u32 tag)
{
    s32   rc;
    u32   count;
    u8   *base;
    u8   *altBase;
    Slot *slot;
    u32   scaled;

    rc = FUN_080089b0(&gRam0201D6D0, req);
    if (rc != 0)
        return 1;

    req->handle = FUN_08008a28(&gRam0201D6D0, req->id, req);
    if (req->handle == 0)
        return 0;

    if (req->kind > KIND_MAX)
        return 1;

    count = gRam02022ABC;
    if (count > QUEUE_MAX)
        return 1;

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
    return 1;
}

/* 0x08013010 — ROM'da QueueDrawRequest'in birebir ikinci kopyasi. */
u32 QueueDrawRequestDup(Req *req, u32 tag)
{
    s32   rc;
    u32   count;
    u8   *base;
    u8   *altBase;
    Slot *slot;
    u32   scaled;

    rc = FUN_080089b0(&gRam0201D6D0, req);
    if (rc != 0)
        return 1;

    req->handle = FUN_08008a28(&gRam0201D6D0, req->id, req);
    if (req->handle == 0)
        return 0;

    if (req->kind > KIND_MAX)
        return 1;

    count = gRam02022ABC;
    if (count > QUEUE_MAX)
        return 1;

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
    return 1;
}
