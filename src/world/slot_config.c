/* Yuva yapilandirmasi — 0x0803C050-0x0803C0E3
 *
 * Iki yuva blogu var: birincil 0x02000F10, ikincil 0x02001140. Her ikisi de
 * +0 deger, +4 tur, +12 alan ve +32 isleyici isaretcisi tutuyor. Ikincil
 * yuva yalnizca gGameState[12] kuruluyken gecerli.
 *
 * Isleyici adresleri Thumb biti kurulu saklandigi icin #define ile tam
 * deger veriliyor (src/world/actor_states.c ile ayni gerekce).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_config.c
 */

#include "gba_types.h"

typedef struct Slot Slot;
typedef void (*SlotHandler)(void);

#define HANDLER_LINKED   ((SlotHandler)0x080397ED)   /* 0x080397EC */
#define HANDLER_PLAIN    ((SlotHandler)0x08038A61)   /* 0x08038A60 */

#define SLOT_KIND_LINKED 2
#define SLOT_PRIMARY     1
#define SLOT_SECONDARY   2

struct Slot {
    u32         value;          /* +0x00 */
    u32         kind;           /* +0x04 */
    u8          pad08[4];
    u32         unk0C;          /* +0x0C */
    u8          pad10[16];
    SlotHandler handler;        /* +0x20 */
};

extern Slot  gRam02000F10;      /* birincil */
extern Slot  gRam02001140;      /* ikincil  */
extern Slot *gSessionPtr;       /* 0x02000F04 */
extern u8    gGameState[];

/* 0x0803C050 */
void ConfigureSlot(u32 value, u32 kind, int which)
{
    Slot *slot;

    if (which == 0) {
        slot = &gRam02000F10;
        slot->value = value;
        slot->kind = kind;
        if (kind == SLOT_KIND_LINKED) {
            slot->handler = HANDLER_LINKED;
            return;
        }
    } else {
        slot = &gRam02001140;
        slot->value = value;
        slot->kind = kind;
        if (kind == SLOT_KIND_LINKED) {
            slot->handler = HANDLER_LINKED;
            return;
        }
    }

    slot->handler = HANDLER_PLAIN;
}

/* 0x0803C090 */
u32 GetActiveSlotValue(void)
{
    if (gGameState[12] == 0)
        return gRam02000F10.value;

    return gSessionPtr->value;
}

/* 0x0803C0B4 */
u32 GetSlotField(int which)
{
    if (which == SLOT_PRIMARY)
        return gRam02000F10.unk0C;
    if (which != SLOT_SECONDARY)
        return 0;
    if (gGameState[12] == 0)
        return 0;

    return gRam02001140.unk0C;
}
