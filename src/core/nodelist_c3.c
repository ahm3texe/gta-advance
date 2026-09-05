/* Alan girisini kosullardan gecirip dugume baglama — 0x08052C68-0x08052CF7
 *
 * ROM tablosundaki (0x08D49C00 +0x24) 36 baytlik giris dizisinden index'inci
 * girisi aliyor ve dort kapidan geciriyor:
 *   1. gRam020004A0 acikken girisin +0x23 bayrak baytinda 8 biti varsa cik
 *   2. giris maskesi (+0x1C) -1 degilse ve gRam02030C00 ile kesismiyorsa cik
 *   3. +0x18 alan bayragi kuruluysa (IsAreaFlagSet) cik
 *   4. +0x1A kimligi FUN_080552d4'u gecemiyorsa cik
 * Hepsi gecilirse sirali dugum listesinde (gNodeListHead) index aranir;
 * bulunan dugumun +0x0B baytinin ust yarisi 0x10 ise FUN_08052988 cagrilir.
 *
 * Havuz duzeni (4 sozcuk, 0x08052CE8'de):
 *     0x08D49C00  ROM tablo taban yapisi, +0x24 giris dizisi isaretcisi
 *     0x020004A0  bayrak kapisinin anahtari
 *     0x02030C00  maske kapisinin anahtari
 *     0x02035A70  gNodeListHead
 *
 * Taban ROM'da DUZ yukleniyor (`ldr r7,=0x08D49C00` + `ldr r0,[r7,#36]`) ve
 * r7'de tutuluyor -> dizi aritmetigi degil YAPI UYESI erisimi. Olculdu:
 * `extern Bank gAreaBank; gAreaBank.entries` tam bu ikiliyi uretiyor,
 * `((Bank*)0x08D49C00)->entries` ise ofseti havuz sabitine katliyor.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * YENI SEMBOLLER: 0x08D49C00 (gAreaBank), 0x020004A0 (gRam020004A0),
 * 0x02030C00 (gRam02030C00) data/ram_map.csv'de YOK. Baska ajanlar ayni
 * dosyaya yazdigi icin oraya eklenmedi; adresler bu dosyada dosya kapsamli
 * Semboller data/ram_map.csv'de kayitli (birlestirme sirasinda eklendi;
 * ajan calisirken ortak dosyaya yazmasi yasakti ve gecici olarak
 * `asm(".equ ...")` kullanmisti -- o kacamak kaldirildi).  Boylece kural 1
 * (RAM/ROM adresleri extern sembol olmali) korunuyor: derleyici taban+ofset
 * katlamasi yapamiyor. ram_map'e eklendiklerinde bu uc satir silinebilir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c3.c
 */

#include "gba_types.h"

/* Bkz. dosya basi: ram_map'e yazamadigimiz uc adres. */

/* 36 baytlik alan girisi; sadece kullanilan alanlar adlandirildi. */
typedef struct AreaEntry {
    u8  unk00[0x18];        /* 0x00 */
    u16 areaFlag;           /* 0x18 */
    u16 checkId;            /* 0x1A */
    s32 mask;               /* 0x1C */
    u8  unk20[3];           /* 0x20 */
    u8  flags;              /* 0x23 */
} AreaEntry;

typedef struct AreaBank {
    u8         unk00[0x24]; /* 0x00 */
    AreaEntry *entries;     /* 0x24 */
} AreaBank;

/* src/world/node_search.c'deki Node ile ayni yerlesim; +0x0A/+0x0B eklendi. */
typedef struct Node {
    struct Node *next;      /* 0x00 */
    u8           pad04[4];
    u16          id;        /* 0x08 */
    u8           pad0A;     /* 0x0A */
    u8           kind;      /* 0x0B */
} Node;

extern AreaBank gAreaBank;
extern u32      gRam020004A0;
extern u32      gRam02030C00;
extern Node    *gNodeListHead;

extern u32   IsAreaFlagSet(s32 index);
extern u32   FUN_080552d4(s32 id);
extern Node *FUN_080543d0(Node **head, s32 index);
extern void  FUN_08052988(Node *node, AreaEntry *entry);

/* 0x08052C68 */
void LinkAreaEntryIfEligible(s32 index)
{
    AreaEntry *entry;
    Node      *node;

    entry = &gAreaBank.entries[index];

    if (gRam020004A0 != 0) {
        if ((entry->flags & 8) != 0)
            return;
    }
    if (entry->mask != -1) {
        if ((entry->mask & gRam02030C00) == 0)
            return;
    }
    if (entry->areaFlag != 0) {
        if (IsAreaFlagSet(entry->areaFlag) != 0)
            return;
    }
    if (entry->checkId != 0) {
        if (FUN_080552d4(entry->checkId) == 0)
            return;
    }

    node = FUN_080543d0(&gNodeListHead, index);
    if (node == 0)
        return;
    if ((node->kind & 0xF0) != 0x10)
        return;

    FUN_08052988(node, &gAreaBank.entries[index]);
}
