/* Slot marker, value range and ROM byte table — 0x08065518-0x08065573
 *
 * Three functions around slot A. The first finds an entity's owner and writes
 * slot +0x25. The second checks whether the high halfword at A +0x4C lies in
 * the supplied range. The third reads one byte from ROM table 0x08F72620.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_range_query.c
 */

#include "gba_types.h"

#define SELECT_A 1

typedef struct PlayerSlot {
    u8  pad00[0x25];
    u8  marked;                 /* +0x25 */
    u8  pad26[0x4c - 0x26];
    u32 packed;                 /* +0x4C: high halfword holds a position value */
} PlayerSlot;

typedef struct Actor {
    u8    pad00[0x64];
    void *owner;                /* +0x64 */
} Actor;

extern u32       GetOwnerSlot(void *entity);
extern void     *SelectSlotAB(u32 which);
extern const u8  gRom08F72620[];

/* 0x08065518 */
void SetOwnerMark(Actor *actor, s32 value)
{
    PlayerSlot *slot;

    if (value != 0) {
        slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
        if (slot != 0)
            slot->marked = value;
    }
}

/* 0x08065538 */
s32 IsSlotValueInRange(s32 unused, u16 low, u16 high)
{
    PlayerSlot *slot;

    /* Early-exit chain REQUIRED (rule 51). A single && gives low and high the
     * same lifetime (13) and priority, so allocno order swaps r4/r5. This
     * form extends low's lifetime to 14 and lowers its priority: high is
     * allocated first and receives r4, as in the ROM. */
    slot = (PlayerSlot *)SelectSlotAB(SELECT_A);
    if (slot == 0)
        return 0;
    if (slot->packed == 0)
        return 0;
    if ((slot->packed >> 16) < low)
        return 0;
    if ((slot->packed >> 16) >= high)
        return 0;

    return 1;
}

/* 0x08065568 */
u8 LookupRomByte(s32 index)
{
    return gRom08F72620[index];
}
