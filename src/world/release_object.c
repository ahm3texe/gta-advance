/* Releasing an object (unlinking it from the doubly linked list) —
 * 0x08013ABC-0x08013B9B
 *
 * One of the most called helpers in the project (formerly FUN_08013abc).  If
 * the +0x3C/+0x40 links are 0xFDFDFDFD (not linked) it returns immediately.
 * If the +0x27 "owned" byte is 1, the +0x18 node is freed with UnlinkToFree;
 * if bit0 of the +0x20 flag is set, +0x34 goes to FUN_08013308 when it is
 * present and +0x38 to FUN_0801362c.  If there is a +0x44 sub-object, the same
 * work is done for it (its own +0x44's +0x44 goes to FUN_08015110 first) and
 * it is handed back to the gRam020110C0 pool with FUN_0800c804.  Then it is
 * unlinked from the doubly linked list headed at gRam020230A0; the links are
 * set to 0xFDFDFDFD and ownership to 0.
 *
 * TWO MEASUREMENTS: the sub-object must be tested through the field first and
 * taken into a local AFTERWARDS (`if (obj->child != 0) { child = obj->child;
 * ...}`; the ROM loads it into r1 first and copies it to r5 after the test).
 * The unlinking must be written WITH DIRECT FIELD EXPRESSIONS: for
 * `obj->next->prev = obj->prev` the ROM RE-READS both fields (the intervening
 * pointer store defeats CSE); written with prev/next locals it diverges by 7
 * instructions.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/release_object.c
 */

#include "gba_types.h"
#define UNLINKED 0xFDFDFDFD
typedef struct Obj {
    u8 pad00[0x18]; void *node; u8 pad1c[4]; u16 flags20; u8 pad22[5]; u8 owned; u8 pad28[0xC];
    void *unk34; void *unk38; struct Obj *prev; struct Obj *next; struct Obj *child;
} Obj;
extern Obj *gRam020230A0;
extern u32  gRam020110C0[];
extern void UnlinkToFree(void *node);
extern void FUN_08013308(void *p);
extern void FUN_0801362c(void *p);
extern void FUN_08015110(void *p);
extern void FUN_0800c804(u32 *dest, Obj *value);
void ReleaseObject(Obj *obj)
{
    Obj *child; u32 owned;
    if (obj->prev == (Obj *)UNLINKED)
        return;
    if (obj->next == (Obj *)UNLINKED)
        return;
    owned = obj->owned;
    if (owned == 1) {
        UnlinkToFree(obj->node);
        obj->node = 0;
        owned = owned & obj->flags20;
        if (owned != 0 && obj->unk34 != 0)
            FUN_08013308(obj->unk34);
        FUN_0801362c(obj->unk38);
    }
    if (obj->child != 0) {
        child = obj->child;
        if (child->child != 0)
            FUN_08015110(child->child);
        owned = child->owned;
        if (owned == 1) {
            UnlinkToFree(child->node);
            child->node = 0;
            owned = owned & child->flags20;
            if (owned != 0 && child->unk34 != 0)
                FUN_08013308(child->unk34);
            FUN_0801362c(child->unk38);
        }
        FUN_0800c804(gRam020110C0, child);
        obj->child = 0;
    }
    if (obj->prev != 0) {
        if (obj->next != 0) {
            obj->prev->next = obj->next;
            obj->next->prev = obj->prev;
        } else {
            obj->prev->next = obj->next;
            gRam020230A0 = obj->prev;
        }
    } else {
        if (obj->next != 0)
            obj->next->prev = obj->prev;
        else
            gRam020230A0 = obj->next;
    }
    obj->next = (Obj *)UNLINKED;
    obj->prev = (Obj *)UNLINKED;
    obj->owned = 0;
}
