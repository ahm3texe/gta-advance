/* Alan girisini yeniden kurup dugumu geri veriyor. 0x080526B8, 152 bayt.
 *
 * FUN_08052C68 (src/core/nodelist_c3.c) ile kardes: ayni `gRam020004A0
 * acikken bayrak biti 8 kapiyor` korumasini ve ayni `(kind & 0xF0) == 0x10`
 * sinamasini kullaniyor. Farki, kapiya takilmayan durumda dugumun +0x0B
 * bayraklarindan iki bit temizleyip sayaci yeniden kurmasi ve dugumu
 * DONDURMESI.
 *
 * DIKKAT -- iki AYRI liste var. Burada gecilen bas isaretcisi
 * gList02035A80 (0x02035A80); nodelist_c3.c'nin kullandigi gNodeListHead
 * (0x02035A70) DEGIL. ram_map notu ikisinin ayri nesneler oldugunu
 * kaydediyor; adresleri komsu oldugu icin kolayca karistirilir.
 *
 * gAreaBank burada +0x04 (sayac) ve +0x1C (64 baytlik kayit dizisi) olarak
 * goruluyor; nodelist_c3.c ayni sembolu +0x24'teki 36 baytlik giris dizisi
 * olarak goruyor. Ikisi de dogru: her translation unit kendi yerel gorunumune
 * cast ediyor (bkz. include/ram_symbols.h basligi).
 *
 * Maske: ROM `movs r0,#13 / negs r0,r0` ile -13 kuruyor.  Burada alanin
 * isaretliligi FARK ETMIYOR (u8 ile de eslesiyor), cunku `&= ~12` bileşik
 * atamasinda islem int genisliginde yapiliyor ve `~12` zaten -13.
 * Kural 47'nin gecerli oldugu durum baska: sonuc DAR tipe indirgenerek
 * kullanildiginda (ResetActor'de oldugu gibi) u8 alanda sabit 0xF0'a
 * katlaniyor.
 *
 * ELENEN YAZIM: sonucu once `s32` yerele alip alana geri yazmak.  agbcc
 * o durumda depolamadan once `lsls #24 / asrs #24` ile normallestirme
 * sokuyor -- iki fazla komut, ROM'da yok.  Dogrudan bileşik atama dogru.
 *
 * Donus tipi: ROM cikista `adds r0,r4,#0` yapip `pop {r1}; bx r1` ile
 * donuyor, yani r0 CANLI -> deger donduren fonksiyon.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b6.c
 */

#include "gba_types.h"

#define KIND_CLEAR  12          /* +0x0B'de temizlenen iki bit */
#define KIND_MASK   0xF0
#define KIND_READY  0x10
#define SPAN_NUM    15
#define SPAN_SHIFT  2
#define SPAN_BIAS   9
#define BITS_SKIP   0x80
#define RECORD_SIZE 0x40

typedef struct Shape {
    u8  pad00[10];
    u16 span;                   /* 0x0A */
} Shape;

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    pad0A;
    s8    kind;                 /* 0x0B */
    u8    pad0C[8];
    s16   timer;                /* 0x14 */
    u8    pad16[2];
    u32   bits;                 /* 0x18 */
    u8    pad1C[8];
    u32   value;                /* 0x24 */
    u8    pad28[4];
    Shape *shape;               /* 0x2C */
} Node;

/* 64 baytlik alan kaydi; yalnizca kullanilan alan adlandirildi. */
typedef struct Record {
    u8 pad00[0x3d];
    u8 flags;                   /* 0x3D */
    u8 pad3e[2];
} Record;

typedef struct AreaBank {
    u8      pad00[4];
    s32     count;              /* 0x04 */
    u8      pad08[0x14];
    Record *records;            /* 0x1C */
} AreaBank;

extern AreaBank gAreaBank;
extern u32      gRam020004A0;
extern Node    *gList02035A80;

extern Node *FUN_080543d0(Node **head, s32 index);
extern void  FUN_080521c4(Node *node, Record *record);

/* 0x080526B8 */
Node *RebuildAreaEntry(s32 index, u32 value)
{
    Node *node;

    if (index >= gAreaBank.count) return 0;
    if (gRam020004A0 != 0) {
        if ((gAreaBank.records[index].flags & 8) != 0) return 0;
    }

    node = FUN_080543d0(&gList02035A80, index);

    node->kind &= ~KIND_CLEAR;
    if ((node->kind & 2) != 0) {
        FUN_080521c4(node, &gAreaBank.records[index]);
    }

    if ((node->kind & KIND_MASK) == KIND_READY) {
        node->value = value;
        node->timer = ((node->shape->span * SPAN_NUM) >> SPAN_SHIFT) + SPAN_BIAS;
        if ((node->bits & BITS_SKIP) != 0) node->timer = 0;
    }
    return node;
}
