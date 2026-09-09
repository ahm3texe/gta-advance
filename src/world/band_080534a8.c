/* Setting up the object's id/slot records and advancing them each round
 * 0x080534A8-0x0805364F  (424 bytes; the last 12 are the literal pool)
 *
 * STATUS: BYTE-MATCHING (424/424 bytes, 203/203 instructions) — on the first
 * attempt.
 *
 * The structure family is THE SAME as in nodelist_d1.c
 * (ClearObjectIdsAndSlots) and nodelist_d2.c (ClearAreaIdArrays); the struct
 * layouts were taken from there.  Obj: the +0x0B bitfield
 * (dirty:1/pad:3/level:4, SIGNED), the +0x18 header, the +0x20 id array, the
 * +0x24 60-byte Area records (identical to d1); the +0x0A/+0x16/+0x17 fields
 * were measured from this body.
 *
 * THE FLOW AS READ FROM THE ROM:
 *   1. header = obj->header is read FIRST (`ldr r0,[r7,#24]` ahead of the
 *      counter), then the +0x16 countdown: if `== 5`, decrement and CONTINUE;
 *      if `> 0`, decrement and RETURN.  The value uses `ldrb` and the
 *      comparison `ldrsb` -> the field is `s8`, NOT a bitfield.
 *   2. If +0x17 is non-zero, ClearObjectIdsAndSlots and return.
 *   3. If not dirty and the level is > 0: FindFreeSlotRun(idCount) ->
 *      obj->ids, FindNthFreeSlot(recordCount) -> obj->records.  Both NULL
 *      checks test the RETURNED value (the rule: a call's result into a local
 *      first), and if the second fails, obj->ids is re-read FROM THE FIELD and
 *      FillSlotsWithNone is called.
 *   4. The id copy loop: `*dst = *src; LinkAreaEntryIfEligible(*dst)` — the
 *      call argument is re-read FROM THE DESTINATION (the same as in
 *      nodelist_a6.c).
 *   5. The record setup loop: for each Area, save/disable IME + zero 60 bytes
 *      with DMA3 (0x8500000F = 15 words) + a dead control read + restore IME;
 *      then a 20-byte AreaInfo struct assignment (ldmia/stmia 3+2),
 *      GetEntrySlot(area->info.id) -> +0x28, `area->level = 0`
 *      (`movs #15 / ands`).
 *   6. obj->dirty = 1, obj->slot = 0xFF.  Return if not dirty.
 *   7. FUN_08051d10(obj, header->table, header->mode), then FUN_08053030 for
 *      each record; if the level is != 0 (`movs #240 / ands`, rules 24/26) and
 *      listC is != 0, a FUN_08056048 loop over listC[j].
 *
 * TYPE MEASUREMENTS:
 *   - The outer loop counters are `u32` (the ROM has `bcs`/`bcc`, UNSIGNED).
 *   - The inner loop counter is `int` (the ROM has `bge`/`blt`; the u16 countC
 *     promotes to int).
 *   - The REVERSE of rule 35: `pop {r1}; bx r1` + `movs r0,#0` -> the return
 *     type is `int` and every exit is `return 0`.
 *   - The REG_IME literal is hoisted out of the loop while the REG_DMA3 base
 *     stays inside; that falls out of the existing volatile qualifiers in
 *     gba_io.h on its own, no intervention was needed.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/band_080534a8.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* enable | 32-bit unit | fixed source | 15 words = 60 bytes */
#define DMA_CLEAR_AREA  0x8500000F

#define OBJ_SLOT_INIT   0xFF
#define TIMER_RELOAD    5

/* The same as SlotDesc in nodelist_d2.c; only +0x0A is read from it here. */
typedef struct SlotDesc {
    u8  pad00[10];
    u16 countC;                 /* +0x0A */
} SlotDesc;

/* The 20-byte record in the definition bank; copied to the Area's +0x14. */
typedef struct AreaInfo {
    u16 id;                     /* +0x00 */
    u8  pad02[18];
} AreaInfo;

/* The Area from nodelist_d2.c (a 60-byte slot record). */
typedef struct Area {
    u8        pad00[11];
    u8        pad0B : 4;        /* +0x0B bit 0..3 */
    s8        level : 4;        /* +0x0B bit 4..7 */
    u8        pad0C[8];
    AreaInfo  info;             /* +0x14 (20 bytes) */
    SlotDesc *desc;             /* +0x28 */
    u8        pad2C[12];
    struct Area *listC;         /* +0x38 */
} Area;

/* The wide form of ObjHeader from nodelist_d1.c. */
typedef struct ObjHeader {
    u8        pad00[5];
    u8        idCount;          /* +0x05 */
    u8        mode;             /* +0x06 */
    u8        recordCount;      /* +0x07 */
    u16      *ids;              /* +0x08 */
    void     *table;            /* +0x0C */
    AreaInfo *infos;            /* +0x10 */
} ObjHeader;

/* The Obj from nodelist_d1.c; its +0x0A/+0x16/+0x17 fields were measured
   here. */
typedef struct Obj {
    u8         pad00[10];
    u8         slot;            /* +0x0A */
    u8         dirty : 1;       /* +0x0B bit 0    */
    u8         pad0B : 3;       /* +0x0B bit 1..3 */
    s8         level : 4;       /* +0x0B bit 4..7 */
    u8         pad0C[10];
    s8         timer;           /* +0x16 */
    u8         teardown;        /* +0x17 */
    ObjHeader *header;          /* +0x18 */
    u8         pad1C[4];
    u16       *ids;             /* +0x20 */
    Area      *records;         /* +0x24 */
} Obj;

extern void  ClearObjectIdsAndSlots(Obj *obj);
extern u16  *FindFreeSlotRun(int count);
extern Area *FindNthFreeSlot(u32 count);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  LinkAreaEntryIfEligible(s32 index);
extern u8   *GetEntrySlot(int index);
extern void  FUN_08051d10(Obj *obj, void *table, u32 mode);
extern void  FUN_08053030(Area *area);
extern void  FUN_08056048(Area *area);

/* 0x080534A8 */
int FUN_080534a8(Obj *obj)
{
    ObjHeader *header;
    Area      *area;
    Area      *records;
    AreaInfo  *info;
    u16       *dst;
    u16       *src;
    u16       *slots;
    u32        i;
    int        j;
    u32        fill;
    u16        ime;

    header = obj->header;

    if (obj->timer == TIMER_RELOAD) {
        obj->timer--;
    } else if (obj->timer > 0) {
        obj->timer--;
        return 0;
    }

    if (obj->teardown != 0) {
        ClearObjectIdsAndSlots(obj);
        return 0;
    }

    if (obj->dirty == 0) {
        if (obj->level > 0) {
            slots = FindFreeSlotRun(header->idCount);
            obj->ids = slots;
            if (header->idCount != 0 && slots == 0)
                return 0;

            records = FindNthFreeSlot(header->recordCount);
            obj->records = records;
            if (header->recordCount != 0 && records == 0) {
                if (obj->ids != 0)
                    FillSlotsWithNone(obj->ids, header->idCount);
                return 0;
            }

            dst = obj->ids;
            src = header->ids;
            for (i = 0; i < header->idCount; i++, dst++, src++) {
                *dst = *src;
                LinkAreaEntryIfEligible(*dst);
            }

            info = header->infos;
            area = obj->records;
            for (i = 0; i < header->recordCount; i++, info++, area++) {
                ime = REG_IME;
                REG_IME = 0;
                fill = 0;
                REG_DMA3.src = &fill;
                REG_DMA3.dst = area;
                REG_DMA3.control = DMA_CLEAR_AREA;
                REG_DMA3.control;
                REG_IME = ime;

                area->info = *info;
                area->desc = (SlotDesc *)GetEntrySlot(area->info.id);
                area->level = 0;
            }

            obj->dirty = 1;
            obj->slot = OBJ_SLOT_INIT;
        }
        if (obj->dirty == 0)
            return 0;
    }

    FUN_08051d10(obj, header->table, header->mode);

    area = obj->records;
    for (i = 0; i < header->recordCount; i++, area++) {
        FUN_08053030(area);
        if (area->level != 0 && area->listC != 0) {
            for (j = 0; j < area->desc->countC; j++)
                FUN_08056048(&area->listC[j]);
        }
    }

    return 0;
}
