/* Nesneyi birakma (cift bagli listeden cikarma) — 0x08013ABC-0x08013B9B
 *
 * Projede en cok cagrilan yardimcilardan biri (eski adi FUN_08013abc).
 * +0x3C/+0x40 baglari 0xFDFDFDFD ise (bagli degil) hemen doner. +0x27
 * "sahipli" bayti 1 ise +0x18 dugumu UnlinkToFree ile serbest kalir,
 * +0x20 bayraginin bit0'i ve +0x34 doluysa FUN_08013308, +0x38 icin
 * FUN_0801362c. +0x44 alt nesnesi varsa onun icin de ayni is (once
 * +0x44'unun +0x44'u FUN_08015110) ve gRam020110C0 havuzuna FUN_0800c804
 * ile geri verilir. Sonra gRam020230A0 basli cift bagli listeden cikarilir,
 * baglar 0xFDFDFDFD, sahiplik 0.
 *
 * IKI OLCUM: alt nesne once alan uzerinden sinanip SONRA yerele alinmali
 * (`if (obj->child != 0) { child = obj->child; ...}`; ROM once r1'e
 * yukleyip testten sonra r5'e kopyaliyor). Liste cikarma DOGRUDAN ALAN
 * IFADELERIYLE yazilmali: ROM `obj->next->prev = obj->prev` icin iki
 * alani da YENIDEN okuyor (araya giren isaretci store'u CSE'yi bozuyor);
 * prev/next yerelleriyle yazilinca 7 komut sapiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/release_object.c
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
