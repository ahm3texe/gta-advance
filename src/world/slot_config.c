/* Slot configuration — 0x0803C050-0x0803C0E3
 *
 * Primary block 0x02000F10 and secondary block 0x02001140 each hold +0 value,
 * +4 type, +12 field, and +32 handler pointer. The secondary slot is valid
 * only when gGameState[12] is set.
 *
 * Handler addresses retain the Thumb bit, so exact-value #defines are used
 * for the same reason as in actor_states.c.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_config.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

typedef struct Slot Slot;
typedef void (*SlotHandler)(void);

#define HANDLER_LINKED   ((SlotHandler)0x080397ED)   /* 0x080397EC */
#define HANDLER_PLAIN    ((SlotHandler)0x08038A61)   /* 0x08038A60 */

#define SLOT_KIND_LINKED 2
#define SLOT_PRIMARY     1
#define SLOT_SECONDARY   2

struct Slot {
    /* +0x00 is a POINTER: menu_screen.c loads and dereferences the same word
     * (ldr r1,[r0] + ldrb r2,[r1,#8]). u32 would emit the same bytes but
     * would obscure the meaning. */
    void       *entry;          /* +0x00 */
    u32         kind;           /* +0x04 */
    u8          pad08[4];
    u32         unk0C;          /* +0x0C */
    u8          pad10[16];
    SlotHandler handler;        /* +0x20 */
};

extern Slot *gSessionPtr;       /* 0x02000F04 */
extern u8    gGameState[];

/* 0x0803C050 */
void ConfigureSlot(void *entry, u32 kind, int which)
{
    Slot *slot;

    if (which == 0) {
        slot = (Slot *)gRam02000F10;
        slot->entry = entry;
        slot->kind = kind;
        if (kind == SLOT_KIND_LINKED) {
            slot->handler = HANDLER_LINKED;
            return;
        }
    } else {
        slot = (Slot *)gRam02001140;
        slot->entry = entry;
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
        return (u32)((Slot *)gRam02000F10)->entry;

    return (u32)gSessionPtr->entry;
}

/* 0x0803C0B4 */
u32 GetSlotField(int which)
{
    if (which == SLOT_PRIMARY)
        return ((Slot *)gRam02000F10)->unk0C;
    if (which != SLOT_SECONDARY)
        return 0;
    if (gGameState[12] == 0)
        return 0;

    return ((Slot *)gRam02001140)->unk0C;
}
