/* Allocating a draw entry — 0x08012E78-0x08012F87
 *
 * If the key lies in ROM (0x08000000..0x08FFFFFF), EWRAM
 * (0x02000000..0x0203FFFF) or IWRAM (0x03000000..0x03007FFF) and the flags
 * are non-zero: for kinds (4,8) with extra bit15 set, the flags are shifted
 * by one.  The key is looked up in the queue (gRam02022AB0: +0 count, +4
 * entries, +8 last index) with FUN_08032434 and returned if found.  Otherwise,
 * if the count has passed 511 the queue is reset, gRam0201F2B0 is cleared by
 * DMA3 (0xE00 words) and LoadEntryTileData is called; then the new entry
 * (28 bytes) is filled in, sorted with FUN_0803232c and the count is bumped.
 *
 * THREE MEASUREMENTS: the search result must be COPIED into `found` first and
 * tested afterwards (copied after the test, agbcc propagates found=0 as a
 * constant and emits a spurious `movs #0 / mov sl`); the function must have a
 * SINGLE return point (`if (e == 0) {...} return e;` -- an early
 * `return found` branch misses the ROM's shared `adds r0,r4` exit, 1
 * instruction); in the allocation block, writing `e->a = e->b = found` (zero)
 * gives the ROM's r8 copy.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/alloc_draw_entry.c
 */

#include "gba_types.h"
#include "gba_io.h"
#define ROM_BASE      0x08000000
#define EWRAM_START   0x02000000
#define IWRAM_START   0x03000000
#define QUEUE_MAX     0x1FF
#define DMA_FILL_CTRL 0x85000E00
typedef struct Entry { u32 a; u32 b; u16 flags; u8 kindA; u8 kindB; u32 key; u8 pad10[12]; } Entry;
typedef struct Queue { u32 count; Entry *entries; s16 last; } Queue;
extern Queue gRam02022AB0;
extern u8    gRam0201F2B0[];
extern Entry *FUN_08032434(Entry *entries, s32 last, u32 key);
extern void   FUN_0803232c(Entry *entries, s16 *last, u32 count);
extern void   LoadEntryTileData(void);
Entry *AllocDrawEntry(u32 key, u32 flags, u8 kindA, u8 kindB, u32 extra)
{
    Queue *q; Entry *found; Entry *e; u16 ime; volatile u32 fill;
    if ((key - ROM_BASE) > 0xFFFFFF && (key - EWRAM_START) > 0x3FFFF && (key - IWRAM_START) > 0x7FFF)
        return 0;
    if (flags == 0)
        return 0;
    if (kindA == 4 && kindB == 8 && (extra & 0x8000))
        flags <<= 1;
    q = &gRam02022AB0;
    e = FUN_08032434(q->entries, q->last, key);
    found = e;
    if (e == 0) {
        if (q->count > QUEUE_MAX) {
            q->count = 0;
            q->last = 0xFFFF;
            ime = REG_IME;
            REG_IME = 0;
            fill = 0;
            REG_DMA3.src = (void *)&fill;
            REG_DMA3.dst = gRam0201F2B0;
            REG_DMA3.control = DMA_FILL_CTRL;
            REG_DMA3.control;
            REG_IME = ime;
            LoadEntryTileData();
        }
        e = &q->entries[q->count];
        e->flags = flags;
        e->kindA = kindA;
        e->kindB = kindB;
        e->key = key;
        e->a = (u32)found;
        e->b = (u32)found;
        FUN_0803232c(q->entries, &q->last, q->count);
        q->count++;
    }
    return e;
}
