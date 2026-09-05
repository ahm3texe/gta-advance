/* Etkin bolge maskesi yenilendiginde alanlari yukle/bosalt
 * 0x0805518C-0x080552D3  (328 bayt; son 4 bayt literal havuzu)
 *
 * Oyuncu(lar)in dunya koordinatindan bir bolge indeksi hesaplanip
 * `1 << indeks` ile maske kuruluyor. Iki oyuncu varsa (gGameState[12])
 * ikinci oyuncunun biti de ekleniyor. Oyuncu nesnesi yoksa maske -1.
 *
 * Yeni maske gRam02030C00'e yaziliyor. Maske -1 ise ya da ONCEKI maskeyle
 * ayniysa is bitiyor. Degistiyse GetRecordIndex()'in verdigi kayit grubunun
 * kimlik dizisi taraniyor; her kimlik icin ROM bankasindaki (0x08D49C00
 * +0x24) 36 baytlik alan girisinin +0x1C maskesine bakiliyor:
 *
 *   - giris maskesi -1              -> atla
 *   - ESKI maskeyle kesisiyorsa     -> alan artik gorunmuyor demektir:
 *       yeni maskeyle de kesisiyorsa atla, yoksa BOSALT
 *       (FindNode + kirli/seviye-1 ise yuvalari temizle + FUN_08055D90)
 *   - eski maskeyle kesismiyorsa    -> alan yeni geldi demektir:
 *       yeni maskeyle kesisiyorsa YUKLE (FUN_08052C68)
 *
 * Bosaltma govdesi src/core/nodelist_c2.c'deki FUN_08055C04 ile birebir
 * ayni: tek `ldrb` uzerinden iki AYRI test (`movs #1 / ands` ve
 * `movs #240 / ands / cmp #16`), ic yuva dongusu, FillSlotsWithNone,
 * `movs #2 / negs` ile kirli bitin temizlenmesi.
 *
 * YAPI IPUCLARI:
 *   - 0x08D49C00 bankasinin +0x2C alani 16 baytlik "kayit grubu" dizisi:
 *     +0x04 u8 kimlik sayisi, +0x08 u16 kimlik dizisi. Taban ROM'da DUZ
 *     yuklenip ofset yukleme komutunda birakiliyor (`ldr r1,=0x08D49C00`
 *     + `ldr r1,[r1,#44]`), yani YAPI UYESI erisimi -> extern nesne.
 *     src/core/nodelist_c3.c ayni bankanin +0x24 alanini kullaniyor.
 *   - Oyuncu yuvalari (gRam02000F10 / gRam02001140) +0x00'da bir
 *     isaretci tasiyor; o nesnenin +0x18'i {s32 x, s32 y} koordinati.
 *
 * OLCULEN DORT AYRINTI (her biri tek basina denendi, dordu de belirleyici):
 *
 * 1) MASKE ADRESI ICIN AYRI YEREL, HEM DE OKUMADAN SONRA BILDIRILEN.
 *    ROM adresi bir kez havuzdan yukluyor (r1), degeri okuyor, sonra
 *    adresi `adds r6,r1,#0` ile KOPYALAYIP cagrilar boyunca r6'da
 *    tutuyor ve sondaki store'u oradan yapiyor. Duz `gRam02030C00 = ...`
 *    yazmak store'un onune ikinci bir havuz yuklemesi koyuyor; kod 2
 *    bayt kisaliyor, havuz hizalama dolgusu 2 bayt ekliyor, net 332/218
 *    fark. `cur = &gRam02030C00;` satirini OKUMADAN SONRA yazmak (kural
 *    19: kaynak sirasi korunur) CSE'ye o atamayi ilk yuklemenin kopyasina
 *    cevirtiyor -- ROM'un `adds r6,r1,#0` komutu tam olarak budur.
 *    Satiri okumadan ONCE yazmak tek pseudo uretir (`ldr r6,havuz`),
 *    kopya cikmaz.
 *
 * 2) SONUC ICIN IKI AYRI YEREL: `bits` ve `active`. ROM'un -1 dali r1'i,
 *    hesaplanan dali r4'u kuruyor ve else dalinin sonunda
 *    `adds r1,r4,#0` ile birlestiriyor; -1 dali bu kopyanin USTUNE
 *    (0x080551E2) atliyor. Tek degiskenle yazildiginda agbcc iki dala da
 *    r4'u verip kopyayi hic uretmiyordu (127/328 fark). `bits` else
 *    dalinda hesaplanip sonunda `active = bits;` yazilinca kopya cikti
 *    ve fark 8'e indi.
 *
 * 3) KIMLIK YERELI `u32` OLMALI, `u16` DEGIL. `u16 id` ile ROM'un tek
 *    `ldrh r7,[r0]`i yerine `ldrh r2,[r0]` + `adds r7,r2,#0` cifti
 *    cikiyor: HImode yerel, hem indeks hesabi hem cagri argumani icin
 *    ayri bir SImode pseudo'ya genisliyor. Bu, src/core/nodelist_d3.c'de
 *    olculen kuralin TERSI yonu -- orada ROM iki register kullandigi icin
 *    `u16` gerekiyordu. Yani kimlik yerelinin genisligi ezberlenmez,
 *    ROM'un kac register kullandigina bakilir.
 *
 * 4) ALAN GIRISI ICIN AYRI ISARETCI YERELI. `mask = gAreaBank.entries[id].mask;`
 *    yazildiginda agbcc once `entries` uyesini yukluyor, sonra id*36'yi
 *    olcekliyor ve toplami INDEKS register'inda birakiyor. ROM tersini
 *    yapiyor: once id*36, sonra `ldr r0,[r0,#36]`, toplam TABAN
 *    register'inda. Aradaki fark 4 komut / 8 bayt. `entry = &gAreaBank.entries[id];`
 *    ayri bir adres hesabi (ADDR_EXPR) urettigi icin sira ve register
 *    dagitimi ROM'unkine oturuyor. Ayni kalibin dogrulanmis ornegi
 *    src/core/nodelist_c3.c'deki FUN_08052C68: orada da
 *    `ldr r7,havuz / lsls / adds / lsls / ldr r0,[r7,#36] / adds r4,r0,r6`.
 *    `(gAreaBank.entries + id)->mask` yazmak ise hicbir seyi degistirmedi
 *    (ayni agac, ayni RTL).
 *
 * DIGER UYGULANAN KURALLAR:
 *   - Kural 1: gRam02030C00 / gNodeListHead / gAreaBank extern sembol.
 *     `((AreaBank *)0x08D49C00)->groups` yazmak ofseti havuz sabitine
 *     katlar (nodelist_c3.c'de olculdu).
 *   - Kural 9/31: sayaclar `int`; ROM `bge` / `blt` (isaretli) uretiyor.
 *   - Kural 24/26: dugumun +0x0B bayti bitfield; `dirty = 0` atamasi
 *     `movs #2 / negs` ciftini veriyor.
 *   - Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *   - `1 << ...` iki yerde de ayni sabitten turuyor; ROM sabiti r5'te
 *     paylasip `adds r4,r5,#0` uretiyor -- ayri ayri `movs #1` yazmak
 *     gerekmiyor, kaynakta iki kez `1 <<` yazmak yetiyor.
 *   - Ic yuva dongusu ve kirli bitin temizlenmesi src/core/nodelist_c2.c
 *     ile birebir ayni kaynaktan geliyor.
 *
 * ESLESME: 328/328 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_d5.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define LEVEL_BASE  1

/* Oyuncu nesnesinin +0x18'indeki dunya koordinati. */
typedef struct Coord {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
} Coord;

typedef struct Racer {
    u8     pad00[0x18];
    Coord *coord;               /* +0x18 */
} Racer;

/* src/core/nodelist_d1.c / nodelist_c2.c ile ayni yerlesim. */
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

/* 36 baytlik alan girisi; src/core/nodelist_c3.c ile ayni. */
typedef struct AreaEntry {
    u8  unk00[0x18];            /* 0x00 */
    u16 areaFlag;               /* 0x18 */
    u16 checkId;                /* 0x1A */
    s32 mask;                   /* 0x1C */
    u8  unk20[3];               /* 0x20 */
    u8  flags;                  /* 0x23 */
} AreaEntry;

/* 16 baytlik kayit grubu. */
typedef struct AreaGroup {
    u8   pad00[4];              /* 0x00 */
    u8   count;                 /* 0x04 */
    u8   pad05[3];              /* 0x05 */
    u16 *ids;                   /* 0x08 */
    u8   pad0C[4];              /* 0x0C */
} AreaGroup;

typedef struct AreaBank {
    u8         unk00[0x24];     /* 0x00 */
    AreaEntry *entries;         /* 0x24 */
    u8         pad28[4];        /* 0x28 */
    AreaGroup *groups;          /* 0x2C */
} AreaBank;

extern AreaBank gAreaBank;
extern u32      gRam02030C00;
extern Node    *gNodeListHead;
extern u8       gGameState[16];

extern u32   FUN_080313f0(s32 x, s32 y);
extern u32   GetRecordIndex(void);
extern Node *FindNode(u32 id);
extern void  FUN_08052750(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);
extern void  FUN_08052c68(s32 index);

/* 0x0805518C */
void FUN_0805518c(void)
{
    AreaGroup *group;
    AreaEntry *entry;
    Node      *node;
    Racer     *racer;
    u16       *slot;
    u32       *cur;
    int        i;
    int        j;
    u32        id;
    s32        mask;
    s32        bits;
    s32        active;
    u32        old;

    old = gRam02030C00;
    racer = *(Racer **)gRam02000F10;
    cur = &gRam02030C00;        /* OKUMADAN SONRA: CSE bunu ilk havuz
                                   yuklemesinin kopyasina cevirir; ust
                                   yorumdaki 1. maddeye bak */
    if (racer == 0) {
        active = -1;
    } else {
        bits = 1 << FUN_080313f0(racer->coord->x, racer->coord->y);
        if (gGameState[12] != 0) {
            racer = *(Racer **)gRam02001140;
            bits |= 1 << FUN_080313f0(racer->coord->x, racer->coord->y);
        }
        active = bits;          /* iki dalin ORTAK store'da birlesmesi icin
                                   ayri yerel; ust yorum 2. madde */
    }
    *cur = active;

    if (active == -1)
        return;
    if (old == active)
        return;

    group = &gAreaBank.groups[GetRecordIndex()];

    for (i = 0; i < group->count; i++) {
        id = group->ids[i];
        /* Ayri isaretci yereli: sira ve register dagitimi ancak boyle
           ROM'unkine oturuyor (ust yorum 4. madde). */
        entry = &gAreaBank.entries[id];
        mask = entry->mask;
        if (mask == -1)
            continue;

        if ((old & mask) != 0) {
            if ((gRam02030C00 & mask) != 0)
                continue;

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
        } else {
            if ((gRam02030C00 & mask) != 0)
                FUN_08052c68(id);
        }
    }
}
