/* TASLAK — 0x08025518, 458 bayt */

#include "gba_types.h"

#define KIND_22   34
#define KIND_33   51
#define KIND_4C   76
#define KIND_65  101

#define VALUE_A  0x000E0000
#define VALUE_B  0x00300000

extern u32 gRom08CA61C0[];
asm(".equ gRom08CA61C0, 0x08CA61C0");
#define TABLE gRom08CA61C0

typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     owner;               /* +0x01 */
    u8     pad02[2];
    u8     sub[38];             /* +0x04 */
    u8     pad2a[34];
    Triple payload;             /* +0x4C */
    u8     pad58[12];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    u32    unk84;               /* +0x84 */
    u8     pad88[8];
    u32    phase;               /* +0x90 */
} Entry;

extern u8  gRam020245A0;
extern u16 gRam02024344;

extern void FUN_08023df0(Triple *p, u32 w, u32 arg2, u32 arg3);
extern void FUN_08060db4(Triple *p, u32 w);
extern void FUN_08013abc(u8 *sub);
extern u32  GetActiveSlotValue(void);
extern void FUN_08035058(u32 slot, u32 id);
extern u32  CreateEntryFromTemplate(Entry *src, u32 unused1, u32 unused2,
                                    u8 kind, u32 phase, u8 owner);

/* 0x08025518 */
u32 FUN_08025518(Entry *e)
{
    Triple *p;
    u32 w;
    u32 tbl;
    u32 ok;
    Triple *p2;
    u32 w2;
    u32 tbl2;
    u32 c47;
    Triple *p3;
    u32 w3;
    u32 tbl3;
    Triple *p4;
    u32 w4;
    u32 tbl4;

    switch (e->kind) {
    case KIND_65:
        c47 = 47;
        if (e->phase == 46 || e->phase == c47)
            w2 = VALUE_A;
        else
            w2 = VALUE_B;
        tbl2 = TABLE[e->owner] << 16;
        p2 = &e->payload;
        FUN_08023df0(p2, w2, e->unk84, tbl2);
        FUN_08060db4(p2, w2);
        break;
    case KIND_33:
    case KIND_4C:
        if (e->kind == KIND_4C || e->phase == 46 || e->phase == 47)
            w = VALUE_A;
        else
            w = VALUE_B;
        tbl = TABLE[e->owner] << 16;
        p = &e->payload;
        FUN_08023df0(p, w, e->unk84, tbl);
        FUN_08060db4(p, w);
        break;
    case KIND_22:
        switch (e->phase) {
        case 28:
            break;
        case 46:
            w3 = VALUE_A;
            tbl3 = TABLE[e->owner] << 16;
            p3 = &e->payload;
            FUN_08023df0(p3, w3, e->unk84, tbl3);
            FUN_08060db4(p3, w3);
            break;
        case 22:
            w4 = VALUE_B;
            tbl4 = TABLE[e->owner] << 16;
            p4 = &e->payload;
            FUN_08023df0(p4, w4, e->unk84, tbl4);
            FUN_08060db4(p4, w4);
            break;
        }
        break;
    }

    if (e->kind == KIND_33 || e->kind == KIND_4C
        || e->phase == 22 || e->phase == 46) {
        if (e->kind == KIND_4C || e->phase == 46) {
            CreateEntryFromTemplate(e, 1, 0, KIND_22, 47, e->owner);
            FUN_08035058(GetActiveSlotValue(), 461);
        } else {
            CreateEntryFromTemplate(e, 1, 0, KIND_22, 7, e->owner);
            FUN_08035058(GetActiveSlotValue(), 258);
        }
        gRam020245A0 = 1;
        gRam02024344 = 0;
    }

    if (e->phase == 12)
        goto yes;
    if (e->phase == 38)
        goto yes;
    ok = 0;
    if (e->phase == 39)
        ok = 1;
    if (ok == 0)
        goto no;
yes:
    return 1;
no:
    FUN_08013abc(e->sub);
    e->active = 0;
    return 0;
}
