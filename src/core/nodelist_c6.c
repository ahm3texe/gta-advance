/* Create an entity on the node and bind it to the primary or secondary slot
 * 0x08053FF4, 292 bytes (16 bytes of literal pool in the middle, 12 at the
 * end).
 *
 * If the primary id (0x020272C8) is occupied AND its entity is set up, the new
 * node is bound to the SECONDARY slot (id 0x02026F34, slot 1, kind 2);
 * otherwise to the PRIMARY slot (id 0x020272C8, slot 0, kind 1). Both branches
 * follow the same order: set the lock, write the id, create the entity,
 * forward it, configure the slot, prepare the sub-object, send two
 * notifications if this is the selected slot, cross-link the node and the
 * entity, clear the lock, return 1.
 *
 * DETAILS MEASURED FROM THE ROM
 * -----------------------------
 * - The epilogue `pop {r1}; bx r1` with `movs r0,#1`: because r0 stays live,
 *   the return address is taken into r1 -- the function RETURNS A VALUE (the
 *   inverse of rule 35). There is no narrowing instruction, so the return type
 *   is 32-bit.
 * - The prologue `push {r4-r7,lr}` + `push {r8,r9}`: six values cross the
 *   calls; r8/r9 are branch-specific (see below).
 * - In the THEN branch the address 0x0202F2DC goes to r9 and the constant 2 to
 *   r8, and that 2 is used both when writing the lock and in the final
 *   `bits |= 2`: agbcc keeps the same constant in a single register through
 *   CSE, so writing a plain `2` twice in the source is enough (a separate
 *   constant local is NOT needed). In the ELSE branch 1 is written to the
 *   lock, so there is no sharing and the final 2 is built with `movs r1,#2` --
 *   the differing instruction counts of the two branches are exactly the trace
 *   of this.
 * - The `adds r2,r0,#0` at entry: the address 0x020272C8 is saved for the
 *   write in the ELSE branch. That copy is consumed BEFORE the calls
 *   (`str r5,[r2]` before the first `bl`), so a caller-saved r2 suffices and no
 *   separate local is needed.
 * - The final `str r0,[r1,#0]` is shared: agbcc merges the two branches' tails
 *   ITSELF (cross-jumping) and each branch copies its own address register
 *   into r1. Sharing the tail IN THE SOURCE is wrong (see below).
 * - The notification gate 0x02000D40 (gSlotSelector) is read with TWO
 *   DIFFERENT instructions:
 *   THEN `ldrh r0,[r7,#0]` + `cmp #1`, ELSE `movs r1,#0; ldrsh r0,[r7,r1]`
 *   + `cmp #0`. Thumb has no immediate-offset form of LDRSH, so in the ELSE
 *   branch the compiler knowingly paid for an extra instruction: the field is
 *   SIGNED. Measured -- making it `u16` drops the ldrsh in the ELSE to an ldrh
 *   and the size falls to 288 (4 SHORT). With `s16` the `== 1` test narrows to
 *   an ldrh while the `== 0` test does not; this is the read-side form of rule
 *   47 (both directions of mask/width are measured).
 *
 * FORMS I TRIED AND ELIMINATED (all measured)
 * -------------------------------------------
 * 1. Writing the shared tail (cross-link + clear the lock + `return 1`) once,
 *    OUTSIDE the if/else: 252 bytes, 40 SHORT. agbcc then reduces not only the
 *    tail but the `node->sub = obj` block to a single copy as well. The ROM
 *    keeps two full copies and shares only the final store; that only comes
 *    out when the tail is written in BOTH branches.
 * 2. Holding the lock address in a SINGLE shared local (`lock` as the same
 *    variable in both branches): 280 bytes, 12 SHORT. One local means one
 *    pseudo, the two branches share a register and the r8/r9 distinction
 *    disappears. Rule 45: a separate local per branch. Equalising it with
 *    SEPARATE locals per branch (`lock` / `lock2`) also gives 292/292 -- that
 *    is, the same code as the direct form below, so the local-free form was
 *    chosen for simplicity.
 * 3. Making `gSlotSelector` a `u16`: 288 bytes, 4 SHORT (see above).
 * 4. Taking the FUN_08038608 result into a separate local: 292/292, NO
 *    DIFFERENCE. The call has a single use anyway, and the inline form gives
 *    the ROM's order.
 *
 * MATCH: 292/292 bytes.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c6.c
 */

#include "gba_types.h"

typedef struct Node Node;
typedef struct Entity Entity;

/* The creation lock: 2 (secondary) or 1 (primary) is written per branch and
 * cleared when the work is done. REQUIRED: a 4-byte record for 0x0202F2DC must
 * be added to data/ram_map.csv (I cannot name it, hence the #define). */
#define gSpawnLock ((u32 *)0x0202F2DC)

#define SLOT_PRIMARY_KIND    1      /* kind given to the primary slot */
#define SLOT_SECONDARY_KIND  2      /* kind given to the secondary slot */
#define NODE_BOUND_BIT       2      /* "bound to entity" bit at +0x18 */

/* The node layout is from the same family as the sibling
 * src/core/nodelist_b3.c: +0x18 flag word, +0x28 sub-object (the entity
 * here). */
struct Node {
    u8      pad00[0x18];
    u32     bits;               /* +0x18 */
    u8      pad1C[12];
    Entity *sub;                /* +0x28 */
};

/* The entity: +0x2C points back at the node (the same field as in nodelist_b3.c). */
struct Entity {
    u8      pad00[0x14];
    void   *unk14;              /* +0x14 sub-object passed to FUN_0801d8a0 */
    u8      pad18[0x14];
    Node   *node;               /* +0x2C */
};

/* The return of FUN_08038608; only its +0x08 field is used, and that field is
 * ConfigureSlot's `kind` argument (src/world/slot_config.c). */
typedef struct SlotEntry {
    u8  pad00[8];
    u32 kind;                   /* +0x08 */
} SlotEntry;

/* A different view of the same 4-byte word: src/world/node_search.c and
 * src/core/nodelist_a7.c know these as `u32` (an id value).
 * Giving a single symbol two extern types trips check_consistency's ram-extern
 * check, so the DECLARED type is kept and the cast happens at the use site
 * (see the header of include/ram_symbols.h). */
extern u32 gRam020272C8;        /* primary id / node */
extern u32 gRam02026F34;        /* secondary id / node */
extern s16   gSlotSelector;     /* 0x02000D40: the active slot selector */

extern Entity    *SubmitObject(void *object, Node *node);   /* 0x08038234 */
extern void       ForwardZeroArg4(Entity *e, u32 arg, u32 zero);
extern SlotEntry *FUN_08038608(void *object);
extern void       ConfigureSlot(Entity *e, u32 kind, int which);
extern void       FUN_0801d8a0(void *sub, u32 zero);
extern void       FUN_08008f74(u32 arg);
extern void       BindActorToCoordSlot(Entity *e, u32 kind, u32 zero);
extern void       FUN_0800a484(u32 zero);

/* 0x08053FF4 */
u32 SpawnNodeObject(Node *node, u32 arg)
{
    Node   *cur;
    Entity *obj;

    cur = (Node *)gRam020272C8;
    if (cur != 0 && cur->sub != 0) {
        /* The primary id is already set: bind to the secondary slot. */
        *gSpawnLock = SLOT_SECONDARY_KIND;
        gRam02026F34 = (u32)node;
        obj = SubmitObject(0, node);
        ForwardZeroArg4(obj, arg, 0);
        ConfigureSlot(obj, FUN_08038608(0)->kind, 1);
        FUN_0801d8a0(obj->unk14, 0);
        if (gSlotSelector == 1)
            FUN_08008f74(arg);
        BindActorToCoordSlot(obj, SLOT_SECONDARY_KIND, 0);
        if (gSlotSelector == 1)
            FUN_0800a484(0);
        /* The tail must be written in full in both branches (eliminated path 1). */
        node->sub = obj;
        node->bits |= NODE_BOUND_BIT;
        obj->node = node;
        *gSpawnLock = 0;
        return 1;
    } else {
        /* The primary slot is free: write the id here. */
        *gSpawnLock = SLOT_PRIMARY_KIND;
        gRam020272C8 = (u32)node;
        obj = SubmitObject(0, node);
        ForwardZeroArg4(obj, arg, 0);
        ConfigureSlot(obj, FUN_08038608(0)->kind, 0);
        FUN_0801d8a0(obj->unk14, 0);
        if (gSlotSelector == 0)
            FUN_08008f74(arg);
        BindActorToCoordSlot(obj, SLOT_PRIMARY_KIND, 0);
        if (gSlotSelector == 0)
            FUN_0800a484(0);
        node->sub = obj;
        node->bits |= NODE_BOUND_BIT;
        obj->node = node;
        *gSpawnLock = 0;
        return 1;
    }
}
