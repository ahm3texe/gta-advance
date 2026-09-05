/* Nesnenin butun kimliklerini ve yuva kayitlarini bosaltma
 * 0x080536BC-0x08053793  (216 bayt; son 4 bayt literal havuzu)
 *
 * Nesnenin +0x18'indeki basligi (sayaclar) ve +0x20'deki u16 kimlik dizisini
 * aliyor. Kimlik dizisinin her girisi icin:
 *   - kimlikten dugum bulunuyor (FindNode),
 *   - dugum "kirli" (bit 0) ve seviyesi 1 ise dugumun kendi yuva dizisindeki
 *     her kimlik icin FUN_08052750 cagriliyor, yuvalar DMA ile bos kimlikle
 *     dolduruluyor ve kirli biti temizleniyor -- bu govde nodelist_c2.c'deki
 *     FUN_08055C04 ile birebir ayni,
 *   - dugum bulunduysa liste basi + kimlik ile FUN_08055D90 cagriliyor.
 * Sonra nesnenin kendi kimlik dizisi bos kimlikle dolduruluyor, +0x24'teki
 * 60 baytlik yuva kayitlarinin her biri FUN_08052CF8'e veriliyor, kayit
 * dizisi ClearSlots ile temizleniyor ve nesnenin kirli biti siliniyor.
 *
 * Yapi ipuclari:
 *   - Cagiran RefreshThenNotify (src/world/refresh_then_notify.c) nesnenin
 *     +0x0B baytini 0xF1 ile maskeleyip 17 ile karsilastiriyor; yani nesne
 *     de dugumler gibi bit0 = kirli, bit4..7 = seviye tasiyor.
 *   - Basligin +0x05 bayti kimlik sayisi, +0x07 bayti yuva kaydi sayisi.
 *   - Yuva kaydi 60 bayt (`adds r5, #60`).
 *
 * OLCULEN AYRINTILAR:
 *   - +0x0B BITFIELD olarak yaziliyor (nodelist_c1.c / nodelist_c2.c ile
 *     ayni): `dirty` testi `movs #1 / ands / cmp #0`, `level == 1` testi
 *     kaydirmasiz `movs #240 / ands / cmp #16`, `dirty = 0` ise
 *     `movs #2 / negs` ciftini veriyor. Maske aritmetigi yazmak ucunu de
 *     bozar.
 *   - Kural 43: sayac ve isaretci birlikte ilerliyor -> ikisi de `for`
 *     artiriminda ve ROM sirasiyla (once `adds r4,#1`, sonra `adds r6,#2`).
 *     Artirimlari ters yazmak (`ids++, i++`) ROM'un sirasini bozuyor.
 *   - Kural 9/31: sayaclar `int`; ROM'un dallari isaretli (`bge` / `blt`).
 *   - Kural 11: kimlik cagrilar boyunca yasiyor (ROM r9'da tutuyor), bu
 *     yuzden ayri yerele aliniyor.
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - Dongu bittikten sonraki FillSlotsWithNone / ClearSlots cagrilarinda
 *     dizi tabani yurutulmus isaretciden degil, nesneden YENIDEN okunuyor
 *     (`ldr r0, [r1, #32]` / `ldr r0, [r2, #36]`).
 *   - `i` her iki donguda da AYNI degisken. Ikinci dongu icin ayri bir
 *     sayac acmak farki 8 bayttan 22 bayta cikardi.
 *   - Bos dugum icin `continue` yaziliyor. Govdeyi `if (node != 0) { ... }`
 *     icine almak ayni komutlari uretiyor ama asagidaki beraberligi ters
 *     cozuyor ve 8 bayt fark birakiyor.
 *
 * BILDIRIM SIRASI BURADA ANLAMLI -- son 8 baytin sebebi buydu:
 *   agbcc'nin dongu gecidi, dongu boyunca tasinan artirimlar icin iki EK
 *   pseudo uretiyor (`i+1` ve `ids+2`; -dg dokumunde 72 ve 73). Ikisi de
 *   refs=4, live_length=48 tasiyor, yani oncelikleri
 *   (floor_log2(4)*4/48 = 0.167) BIREBIR ESIT. global.c beraberligi allocno
 *   NUMARASIYLA coziyor; o numara da kaynaktaki BILDIRIM SIRASINDAN
 *   geliyor. `ids` once bildirilirse onun tasiyicisi `sl`'i kapiyor,
 *   sayacinki yigina tasiyor:
 *       adds r4,#1 / str r4,[sp] / adds r6,#2 / mov sl,r6      (YANLIS)
 *   Sayaclar once bildirilince sira donuyor ve ROM cikiyor:
 *       adds r4,#1 / mov sl,r4  / adds r6,#2 / str r6,[sp]     (DOGRU)
 *   Yani asagidaki bildirim sirasi degistirilemez; `ids`'i en sona almak da
 *   ayni sonucu veriyor -- onemli olan `i`'nin `ids`'ten ONCE bildirilmesi.
 *   Kayan komutlar ayni yerde kaldigi, yalnizca DEPOLARI takas oldugu icin
 *   bu fark hicbir "yeniden yaz" denemesiyle degil, ancak -dg dokumunu
 *   okuyup beraberligi gorerek kapaniyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_d1.c
 */

#include "gba_types.h"

#define LEVEL_BASE  1

typedef struct SlotDesc {
    u8 pad00[6];
    u8 count;                   /* +0x06 */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           pad0A;
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

/* Nesnenin basligi: yalniz iki sayac alani kullaniliyor. */
typedef struct ObjHeader {
    u8 pad00[5];
    u8 idCount;                 /* +0x05 */
    u8 pad06;
    u8 recordCount;             /* +0x07 */
} ObjHeader;

/* Yuva kaydi; yalniz boyu (60 bayt) biliniyor. */
typedef struct ObjRecord {
    u8 pad00[60];
} ObjRecord;

typedef struct Obj {
    u8         pad00[11];
    u8         dirty : 1;       /* +0x0B bit 0    */
    u8         pad0B : 3;       /* +0x0B bit 1..3 */
    s8         level : 4;       /* +0x0B bit 4..7 */
    u8         pad0C[12];
    ObjHeader *header;          /* +0x18 */
    u8         pad1C[4];
    u16       *ids;             /* +0x20 */
    ObjRecord *records;         /* +0x24 */
} Obj;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindNode(u32 id);
extern void  FUN_08052750(u32 a, u32 b);
extern void  ClearAreaIdArrays(ObjRecord *record);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  ClearSlots(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x080536BC */
void ClearObjectIdsAndSlots(Obj *obj)
{
    /* Sira onemli: sayaclar isaretcilerden ONCE gelmeli.
     * Ust yorumdaki "BILDIRIM SIRASI" notuna bak. */
    int        i;
    int        j;
    ObjHeader *header;
    ObjRecord *record;
    Node      *node;
    u16       *ids;
    u16       *slot;
    u32        id;

    header = obj->header;
    ids = obj->ids;

    for (i = 0; i < header->idCount; i++, ids++) {
        id = *ids;
        node = FindNode(id);
        if (node == 0)
            continue;

        if (node->dirty) {
            if (node->level == LEVEL_BASE) {
                slot = node->slots;
                for (j = 0; j < node->desc->count; j++, slot++)
                    FUN_08052750(*slot, 0);
                FillSlotsWithNone(node->slots, node->desc->count);
                node->dirty = 0;
            }
        }

        FUN_08055d90((u32 *)&gNodeListHead, id);
    }

    FillSlotsWithNone(obj->ids, header->idCount);

    record = obj->records;
    for (i = 0; i < header->recordCount; i++, record++)
        ClearAreaIdArrays(record);

    ClearSlots(obj->records, header->recordCount);
    obj->dirty = 0;
}
