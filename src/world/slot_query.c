/* Slot queries — 0x0803C53C-0x0803C5B3
 *
 * Five small queries. Selector 0x02000D40 chooses the active slot: 1 means
 * secondary (0x02001140), otherwise primary (0x02000F10). Full layout is
 * in slot_config.c.
 *
 * The last two pass GetOwnerSlot's result to SelectSlotAB, then read/write
 * the byte at +80 through the returned pointer.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/slot_query.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define SELECTOR_SECONDARY  1
#define SCALE_BASE          1
#define SCALE_WIDE          256
#define UNIT_SIZE           0x10000
#define MARK_VALUE          16

/* +0x00 is a POINTER: menu_screen.c loads and dereferences this word;
 * src/misc/session_node.c types the same target as Context *. u32 would
 * produce the same bytes but obscure the meaning.
 */
typedef struct SlotValue {
    void *entry;                /* +0x00 */
} SlotValue;

typedef struct Target {
    u8 pad00[80];
    u8 mark;                    /* +0x50 */
} Target;

extern u16       gSlotSelector;     /* 0x02000D40 */
extern u8        gUnk02010C60[];

extern u32     GetOwnerSlot(u32 value);
extern Target *SelectSlotAB(u32 value);

/* 0x0803C53C */
u32 GetUnitSize(void)
{
    return UNIT_SIZE;
}

/* 0x0803C544 */
u32 GetActiveSlot(void)
{
    if (gSlotSelector == SELECTOR_SECONDARY)
        return (u32)((SlotValue *)gRam02001140)->entry;

    return (u32)((SlotValue *)gRam02000F10)->entry;
}

/* 0x0803C564 */
int GetScaleStep(void)
{
    int step;
    u8 wide;

    wide = gUnk02010C60[26];
    step = SCALE_BASE;
    if (wide != 0)
        step += SCALE_WIDE - SCALE_BASE;

    return step;
}

/* 0x0803C578 */
void MarkTarget(u32 value)
{
    Target *target;

    target = SelectSlotAB(GetOwnerSlot(value));
    if (target != 0)
        target->mark = MARK_VALUE;
}

/* 0x0803C594 */
u32 IsTargetMarked(u32 value)
{
    Target *target;

    target = SelectSlotAB(GetOwnerSlot(value));
    if (target == 0)
        return 0;
    if (target->mark != 0)
        return 1;

    return 0;
}
