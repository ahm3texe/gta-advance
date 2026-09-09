/* Area (record) loader — 0x08051154-0x080512AF   BYTE-MATCHING
 *
 * It selects a record from the 60-byte record table at 0x08CAC248 by the given
 * index, writes the selection into gRecordIndex and sets up every world
 * subsystem from that record's fields: the node pool, the window, the entry
 * tables, the distance accumulation and so on. It then searches the spawn list
 * in the record's +0x20 field for the FIRST entry with flags == 0; if one is
 * found, the start position is converted to 16.16 fixed point from that entry's
 * u16 x/y values, and otherwise the default (0x18800000, 0x09400000) remains.
 * At the very end the frame chain is turned by hand once, BG1 is enabled, the
 * 0x08CAA3E4 asset is loaded and its pointer put into gRam020303C4, and the
 * +4/+8/+0xC fields of gRecordIndex are set to (0, 3600, 0).
 *
 * NOTES
 * - The `start` local is DEAD in the ROM: it is written to sp+12/+16/+20 and
 *   never read. Because agbcc does not delete the memory stores of a local
 *   AGGREGATE (a struct), those stores stand in the ROM; the same thing CANNOT
 *   be achieved with scalar locals (they are deleted). It is dead code in the
 *   original source too.
 * - Two addresses are UNNAMED in data/ram_map.csv; they were written as raw
 *   addresses in this file (the same form as gFrameDelay in
 *   src/core/frame_dispatch.c). They need to be named:
 *     0x020303C0  4 bytes, 1 is written on entry to the function -> gLoadFlag
 *     0x08CAA3E4  a ROM asset pointer, passed to FUN_08036cac
 * - The loop form was measured: `for (i = 0; found == -1 && i < n; i++)`.
 *   The ORDER of the conditions matters — written `i < n && found == -1`, the
 *   loop test at the bottom comes out in the reverse order. The
 *   `if (... ) { found = i; break; }` form does not match the ROM's shape
 *   either (rule 60).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/band_08051154.c   -> BYTE-MATCHING
 */

#include "gba_types.h"
#include "ram_symbols.h"

typedef struct SpawnPoint {
    u16 unk00;
    u16 unk02;
    u16 x;                      /* +0x04 */
    u16 y;                      /* +0x06 */
    u16 flags;                  /* +0x08 */
    u16 unk0A;
    u32 unk0C;
} SpawnPoint;                   /* 16 byte */

typedef struct SpawnList {
    u16 count;                  /* +0x00 */
    SpawnPoint *items;          /* +0x04 */
} SpawnList;

typedef struct Record {
    u32 unk00;
    u32 unk04;
    u32 unk08;
    u32 unk0C;
    u32 unk10;
    u32 unk14;
    u32 unk18;
    u32 unk1C;
    SpawnList *spawns;          /* +0x20 */
    u32 unk24;
    u32 unk28;
    u32 unk2C;
    u32 unk30;
    u32 unk34;
    u32 unk38;
} Record;                       /* 60 byte */

typedef struct RecordBlock {
    u32 index;                  /* +0x00 */
    u32 unk04;
    u32 unk08;
    u32 unk0C;
} RecordBlock;

typedef struct Position {
    u32 x;
    u32 y;
    u32 z;
} Position;

extern RecordBlock gRecordIndex;
extern Record gRecords[];

#define gLoadFlag (*(u32 *)0x020303C0)
#define gRecordAsset ((void *)0x08CAA3E4)

extern void InitNodePool(void);
extern void FUN_080130d4(void);
extern void FUN_08013520(void);
extern void FUN_08013450(void);
extern void FUN_08014fac(void);
extern void FUN_08042790(void);
extern void ResetAnchorFull(void);
extern void FUN_0805102c(void);
extern void ClearRam0202370C(void);
extern void FUN_0803fe88(u32 a, u32 b);
extern void FUN_08051964(void);
extern void SetWindow(u32 a, u32 b);
extern void FUN_08019d2c(u32 a, u32 b);
extern void StoreRam0201AEE8(u32 a);
extern void FUN_0800cb08(u32 a, u32 b, u32 c, u32 d);
extern void FUN_08010254(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g);
extern void ClearEntryTables(void);
extern void FUN_08035800(void);
extern void FUN_08035cd8(void);
extern void ResetDistanceAccum(void);
extern void FUN_08050130(void);
extern void FUN_08061d34(void);
extern void FUN_0805cecc(void);
extern void FUN_080296a0(u32 a, u32 b);
extern void FUN_0803c20c(void);
extern void InitMenuSession(void);
extern void StepNodeLists(void);
extern void FUN_0803214c(void);
extern void FUN_08008f14(void);
extern void FUN_080104c0(void);
extern void SetBg1Enable(void);
extern void *FUN_08036cac(void *asset, s32 a, s32 b);

/* 0x08051154 */
void FUN_08051154(u32 index)
{
    Record *record;
    SpawnList *list;
    SpawnPoint *spawn;
    s32 found;
    s32 i;
    Position start;

    record = &gRecords[index];
    gRecordIndex.index = index;
    gLoadFlag = 1;

    InitNodePool();
    FUN_080130d4();
    FUN_08013520();
    FUN_08013450();
    FUN_08014fac();
    FUN_08042790();
    ResetAnchorFull();
    FUN_0805102c();
    ClearRam0202370C();
    FUN_0803fe88(record->unk1C, 0);
    FUN_08051964();
    SetWindow(record->unk04, record->unk08);
    FUN_08019d2c(record->unk2C, record->unk30);
    StoreRam0201AEE8(record->unk1C);
    FUN_0800cb08(record->unk34, record->unk04, record->unk08, index);
    FUN_08010254(record->unk0C, record->unk10, record->unk14, record->unk18,
                 record->unk38, record->unk04, record->unk08);
    ClearEntryTables();
    FUN_08035800();
    FUN_08035cd8();
    ResetDistanceAccum();
    FUN_08050130();
    FUN_08061d34();
    FUN_0805cecc();
    FUN_080296a0(record->unk1C, record->unk28);
    FUN_0803c20c();
    InitMenuSession();
    StepNodeLists();
    StepNodeLists();
    FUN_0803214c();

    list = record->spawns;
    found = -1;
    for (i = 0; found == -1 && i < list->count; i++) {
        if (list->items[i].flags == 0)
            found = i;
    }
    if (found != -1)
        spawn = &list->items[found];
    else
        spawn = 0;

    start.x = 0x18800000;
    start.y = 0x09400000;
    if (spawn != 0) {
        start.x = spawn->x << 16;
        start.y = spawn->y << 16;
    }
    start.z = 0;

    FUN_08008f14();
    FUN_080104c0();
    SetBg1Enable();
    *(void **)gRam020303C4 = FUN_08036cac(gRecordAsset, 1, 0);
    gRecordIndex.unk04 = 0;
    gRecordIndex.unk08 = 0xE10;
    gRecordIndex.unk0C = 0;
}
