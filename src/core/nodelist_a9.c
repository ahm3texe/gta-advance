/* Nesnenin kimlik dizisini ve yuva kayitlarini gezip her birini bosaltma.
 * 0x08053794, 158 bayt (literal havuzu YOK).
 *
 * ROM'dan okunan akis:
 *   header = obj->header            (obj +0x18, ROM r9'da tutuyor)
 *   obj->kind'in 2 biti VARSA hicbir sey yapilmadan donuluyor
 *   obj->ids (+0x20) uzerinde header->idCount (+0x05) kadar donuluyor:
 *       node = FindOrRecycleNode(*ids)
 *       node varsa: node->ids (+0x18) ve node->record (+0x14) YAZMACA
 *       aliniyor, ardindan kind'in 2 biti YOK ve 1 biti VARSA dugumun
 *       kendi kimlik dizisi record->idCount (+0x06) kadar gezilip her
 *       kimlik FUN_08052828'e (ikinci arguman 1) veriliyor
 *   sonra obj->records (+0x24) uzerinde header->recordCount (+0x07) kadar
 *   donulup her 60 baytlik kayit FUN_08052ddc'ye veriliyor
 *
 * Bu, nodelist_a3.c'deki FUN_08052ddc'nin ic dongusuyle BIREBIR ayni govde;
 * struct gorunumleri oradan alindi (Node +0x0B kind, +0x14 record, +0x18
 * ids; Record +0x06 idCount). Nesne gorunumu (ObjHeader, ObjRecord)
 * nodelist_d1.c'den alindi.
 *
 * OLCULEN AYRINTILAR:
 *   - Kardes ClearObjectIdsAndSlots (nodelist_d1.c) ILE AYNI DEGIL: orada
 *     +0x0B bitfield olarak yazilmisti (dirty testi ve level == 1 testi),
 *     burada ROM iki ayri maske sinamasi yapiyor (movs #2/ands ve
 *     movs #1/ands, ldrb bir kez) -- yani nodelist_a3.c'deki `u8 kind`
 *     gorunumu dogru olan.
 *   - node->ids ve node->record, bayrak sinamalarindan ONCE yukleniyor
 *     (`ldr r5,[r0,#24]` / `ldr r6,[r0,#20]` sinamalardan once).
 *     OLCULDU: ikisini `if` govdesinin icine almak 160 bayt / 47 fark
 *     veriyor -- yuklemeler ic dongunun preheader'ina kayiyor ve fazladan
 *     bir yazmac tasimasi cikiyor.
 *   - Kural 43: sayac ve isaretci ikisi de `for` artiriminda, ROM'un
 *     sirasiyla (once sayac, sonra isaretci).
 *   - Kural 9/31: sayaclar `int`; ROM'un dallari isaretli (`bge`/`blt`).
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - Sayaclar isaretcilerden ONCE bildiriliyor (nodelist_d1.c'deki
 *     BILDIRIM SIRASI notu): ROM'da dongu tasiyicisi `i+1` r7'yi,
 *     `ids+2` r8'i aliyor, yani sayacinki once dagitiliyor.
 *     OLCULDU: isaretcileri once bildirmek 158 bayt / 10 fark veriyor
 *     (r7 ile r8 takas oluyor, boyut ayni kaliyor).
 *   - Dis dongunun ilk sinamasi ROM'da YALNIZ ALTTA duruyor (`b` ile teste
 *     atlama), ikinci ve ucuncu dongude ise giris korumasi + alttan donen
 *     bicim var. Duz `for` her ucunu de dogru uretiyor; rule 49'daki acik
 *     `goto test` yazimina GEREK YOK -- denenmedi cunku duz bicim zaten
 *     birebir tuttu.
 *
 * DENENIP ELENENLER:
 *   - `if (node != 0) { ... }` yerine `if (node == 0) continue;` -- IKISI DE
 *     eslesiyor, ayni komutlari veriyor. (nodelist_d1.c'de tersi olmustu;
 *     orada `continue` gerekliydi. Buradaki dongude fark yaratmiyor.)
 *   - Yukaridaki iki olcum (yukleme yeri, bildirim sirasi) tek tek geri
 *     alindiginda fark aciliyor; ucuncu bir kaldiraca ihtiyac olmadi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a9.c
 */

#include "gba_types.h"

typedef struct Record {
    u8 pad00[6];
    u8 idCount;                 /* +0x06 */
} Record;

typedef struct Node {
    u8      pad00[0x0b];
    u8      kind;               /* +0x0B */
    u8      pad0C[8];
    Record *record;             /* +0x14 */
    u16    *ids;                /* +0x18 */
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
    u8         pad00[0x0b];
    u8         kind;            /* +0x0B */
    u8         pad0C[12];
    ObjHeader *header;          /* +0x18 */
    u8         pad1C[4];
    u16       *ids;             /* +0x20 */
    ObjRecord *records;         /* +0x24 */
} Obj;

extern Node *FindOrRecycleNode(s32 id);
extern void  FUN_08052828(u16 id, s32 flag);
extern void  FUN_08052ddc(ObjRecord *record);

/* 0x08053794 */
void FUN_08053794(Obj *obj)
{
    /* Sira onemli: sayaclar isaretcilerden ONCE gelmeli. */
    int        i;
    int        j;
    ObjHeader *header;
    ObjRecord *record;
    Node      *node;
    Record    *rec;
    u16       *ids;
    u16       *slot;

    header = obj->header;
    if ((obj->kind & 2) != 0)
        return;

    ids = obj->ids;
    for (i = 0; i < header->idCount; i++, ids++) {
        node = FindOrRecycleNode(*ids);
        if (node != 0) {
            slot = node->ids;
            rec = node->record;
            if ((node->kind & 2) == 0 && (node->kind & 1) != 0) {
                for (j = 0; j < rec->idCount; j++, slot++)
                    FUN_08052828(*slot, 1);
            }
        }
    }

    record = obj->records;
    for (i = 0; i < header->recordCount; i++, record++)
        FUN_08052ddc(record);
}
