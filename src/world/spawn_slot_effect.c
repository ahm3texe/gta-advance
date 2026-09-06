/* Etkin yuva icin efekt uretimi — 0x08065130-0x080651DF
 *
 * Yuvanin kip bayti 4 ise yalnizca bir sinama yapip bir sayaci ust
 * sinirda tutuyor. Degilse baglama sorgusu calistirip 0x10 bitini
 * bekliyor, sonra bir tetikleme cagrisi yapip 51 numarali girisi
 * uretiyor. Hedef tampon ve kimlik alani yuvanin +0x08 bayrak bitlerine
 * gore iki ayri yerden geliyor.
 *
 * Kip bayragi DEGISKENE aliniyor (kural 48): ROM `movs r5,#0 / cmp #4 /
 * movs r5,#1 / cmp r5,#0` uretiyor ve ayni degiskeni sonra +0x2C alanina
 * yaziyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/spawn_slot_effect.c
 */

#include "gba_types.h"

#define MODE_SPECIAL   4
#define FLAG_ALT_BUF   0x30
#define READY_BIT      0x10
#define SPAWN_PHASE    51
#define MARK_VALUE     0x4028
#define TRIGGER_ARG    (200 << 16)
#define COUNTER_LIMIT  (128 << 9)
#define ID_MASK        0x3FF

typedef struct Triple {
    u8 pad00[0x0E];
    u16 id;                     /* +0x0E */
} Triple;

typedef struct AltBuffer {
    u8 pad00[0x1F];
    u8 kind;                    /* +0x1F, alt iki bit kimligi tasiyor */
} AltBuffer;

typedef struct SlotMark {
    u8  pad00[0x28];
    u32 mark;                   /* +0x28 */
    u32 mode;                   /* +0x2C */
} SlotMark;

typedef struct SlotCounter {
    u8  pad00[8];
    s32 value;                  /* +0x08 */
} SlotCounter;

typedef struct ActiveSlot {
    u8           pad00[8];
    u8           flags;         /* +0x08 */
    u8           pad09[0x14 - 0x09];
    void        *context;       /* +0x14 */
    Triple      *primary;       /* +0x18 */
    SlotMark    *mark;          /* +0x1C */
    AltBuffer   *alt;           /* +0x20 */
    u8           pad24[0x30 - 0x24];
    SlotCounter *counter;       /* +0x30 */
} ActiveSlot;

extern u32  GetActiveSlot(void);
extern s32  FUN_080651e0(void *context, s32 mode);
extern u32  FUN_08019db0(void *context, u32 arg, u32 zero1, u32 zero2);
extern void FUN_08037564(ActiveSlot *slot, u32 arg, u32 zero1, u32 zero2);
extern void CreateEntry(Triple *src, u32 arg1, u32 phase, u32 owner);

/* 0x08065130 */
void SpawnSlotEffect(void)
{
    ActiveSlot *slot;
    void       *context;
    s32         special;
    u32         probe;
    Triple     *target;
    u32         id;

    slot = (ActiveSlot *)GetActiveSlot();
    if (slot == 0)
        return;

    context = slot->context;

    special = 0;
    if (slot->flags == MODE_SPECIAL)
        special = 1;

    if (special != 0) {
        if (FUN_080651e0(context, 1) == 0)
            return;
        if (slot->counter->value > COUNTER_LIMIT)
            slot->counter->value = COUNTER_LIMIT;
        return;
    }

    probe = FUN_08019db0(context, *(u32 *)((u8 *)context + 0xEC), 0, 0);
    if ((probe & READY_BIT) == 0)
        return;

    FUN_08037564(slot, TRIGGER_ARG, 0, 0);

    if (slot->mark != 0) {
        slot->mark->mark = MARK_VALUE;
        slot->mark->mode = special;
    }

    if ((slot->flags & FLAG_ALT_BUF) != 0)
        target = (Triple *)((u8 *)slot->alt + 4);
    else
        target = slot->primary;

    if ((slot->flags & FLAG_ALT_BUF) != 0)
        id = ((u32)slot->alt->kind << 30) >> 22;
    else
        id = slot->primary->id & ID_MASK;

    CreateEntry(target, id, SPAWN_PHASE, (u32)slot);
}
