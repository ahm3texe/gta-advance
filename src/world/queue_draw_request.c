/* Queueing a draw request — 0x08012F88-0x08013097
 *
 * TWO FUNCTIONS, ONE BODY.  tools/find_twins.py reported 100% similarity; the
 * ROM bodies are identical instruction by instruction, only the pool addresses
 * and branch targets are shifted -- the same source compiled twice
 * (docs/WORKFLOW.md section 10).
 *
 * The request is handed to FUN_080089b0 first; if that returns non-zero the
 * work is considered done and 1 is returned.  Then a handle is taken from
 * FUN_08008a28; if the handle is 0 it returns 0.  If the kind exceeds 8, or
 * the queue has filled its 128 entries, it returns 1 as well.  Otherwise IME
 * is turned off, an entry is written into the queue (12-byte stride), the
 * counter is bumped and IME is turned back on.
 *
 * TWO MEASUREMENTS:
 *   - The `tag` parameter is NOT u16 but a full word.  Written as u16, a
 *     `lsls #16 / lsrs #16` zero-extension that is not in the ROM is added at
 *     the entry; the narrowing already happens in the final `strh`.
 *   - The +0x04 field must be written from a SEPARATE BASE local (the same
 *     criterion as in release_slot.c): the ROM adds the scale it computed once
 *     to two bases.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/queue_draw_request.c
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

/* 0x08013010 — an exact second copy of QueueDrawRequest in the ROM. */
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
