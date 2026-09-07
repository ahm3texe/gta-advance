/* Nesnenin kimlik/yuva kayitlarini kurup her turda ilerletme
 * 0x080534A8-0x0805364F  (424 bayt; son 12 bayt literal havuzu)
 *
 * DURUM: BYTE-MATCHING (424/424 bayt, 203/203 komut) — ilk denemede.
 *
 * Yapi ailesi nodelist_d1.c (ClearObjectIdsAndSlots) ve nodelist_d2.c
 * (ClearAreaIdArrays) ile AYNI; struct yerlesimleri oradan alindi.
 * Obj: +0x0B bitfield (dirty:1/pad:3/level:4 ISARETLI), +0x18 baslik,
 * +0x20 kimlik dizisi, +0x24 60 baytlik Area kayitlari (d1 ile birebir);
 * +0x0A/+0x16/+0x17 alanlari bu govdeden olculdu.
 *
 * ROM'DAN OKUNAN AKIS:
 *   1. header = obj->header ONCE okunuyor (`ldr r0,[r7,#24]` sayacin
 *      onunde), sonra +0x16 geri sayimi: `== 5` ise azalt ve DEVAM ET,
 *      `> 0` ise azalt ve DON. Deger `ldrb`, karsilastirma `ldrsb` ->
 *      alan `s8`, bitfield DEGIL.
 *   2. +0x17 sifir degilse ClearObjectIdsAndSlots ve don.
 *   3. Kirli degilse ve seviye > 0 ise: FindFreeSlotRun(idCount) ->
 *      obj->ids, FindNthFreeSlot(recordCount) -> obj->records. Iki NULL
 *      kontrolu de DONEN degeri sinaviyor (kural: cagri sonucu once
 *      yerele), ikinci basarisiz olursa obj->ids ALANDAN yeniden okunup
 *      FillSlotsWithNone cagriliyor.
 *   4. Kimlik kopyalama dongusu: `*dst = *src; LinkAreaEntryIfEligible(*dst)`
 *      — cagri argumani HEDEFTEN yeniden okunuyor (nodelist_a6.c ile ayni).
 *   5. Kayit kurma dongusu: her Area icin IME sakla/kapat + DMA3 ile 60
 *      bayti sifirla (0x8500000F = 15 kelime) + olu control okumasi + IME
 *      geri; sonra 20 baytlik AreaInfo struct atamasi (ldmia/stmia 3+2),
 *      GetEntrySlot(area->info.id) -> +0x28, `area->level = 0`
 *      (`movs #15 / ands`).
 *   6. obj->dirty = 1, obj->slot = 0xFF. Kirli degilse don.
 *   7. FUN_08051d10(obj, header->table, header->mode), sonra her kayit
 *      icin FUN_08053030; seviye != 0 (`movs #240 / ands`, kural 24/26)
 *      ve listC != 0 ise listC[j] uzerinde FUN_08056048 dongusu.
 *
 * TIP OLCUMLERI:
 *   - Dis dongu sayaclari `u32` (ROM `bcs`/`bcc`, ISARETSIZ).
 *   - Ic dongu sayaci `int` (ROM `bge`/`blt`, u16 countC int'e yukseliyor).
 *   - Kural 35'in TERSI: `pop {r1}; bx r1` + `movs r0,#0` -> donus tipi
 *     `int`, her cikis `return 0`.
 *   - REG_IME literali dongu disina cikiyor, REG_DMA3 tabani iceride
 *     kaliyor; bu gba_io.h'deki mevcut volatile nitelemeleriyle
 *     kendiliginden olusuyor, mudahale gerekmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_080534a8.c
 */

#include "gba_types.h"
#include "gba_io.h"

/* enable | 32-bit birim | kaynak sabit | 15 kelime = 60 bayt */
#define DMA_CLEAR_AREA  0x8500000F

#define OBJ_SLOT_INIT   0xFF
#define TIMER_RELOAD    5

/* nodelist_d2.c'deki SlotDesc ile ayni; buradan yalniz +0x0A okunuyor. */
typedef struct SlotDesc {
    u8  pad00[10];
    u16 countC;                 /* +0x0A */
} SlotDesc;

/* Tanim bankasindaki 20 baytlik kayit; Area'nin +0x14'une kopyalaniyor. */
typedef struct AreaInfo {
    u16 id;                     /* +0x00 */
    u8  pad02[18];
} AreaInfo;

/* nodelist_d2.c'deki Area (60 bayt yuva kaydi). */
typedef struct Area {
    u8        pad00[11];
    u8        pad0B : 4;        /* +0x0B bit 0..3 */
    s8        level : 4;        /* +0x0B bit 4..7 */
    u8        pad0C[8];
    AreaInfo  info;             /* +0x14 (20 bayt) */
    SlotDesc *desc;             /* +0x28 */
    u8        pad2C[12];
    struct Area *listC;         /* +0x38 */
} Area;

/* nodelist_d1.c'deki ObjHeader'in genisi. */
typedef struct ObjHeader {
    u8        pad00[5];
    u8        idCount;          /* +0x05 */
    u8        mode;             /* +0x06 */
    u8        recordCount;      /* +0x07 */
    u16      *ids;              /* +0x08 */
    void     *table;            /* +0x0C */
    AreaInfo *infos;            /* +0x10 */
} ObjHeader;

/* nodelist_d1.c'deki Obj; +0x0A/+0x16/+0x17 alanlari buradan olculdu. */
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
