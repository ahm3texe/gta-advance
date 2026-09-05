/* Varlik turune gore isleyici + tanim tablosu secimi — 0x0801528C-0x080154D7
 *
 * Iki parametre aliyor: bir varlik kaydi (r0) ve tur numarasi (r1).
 * Tur 0..27 araliginda ise 28 girisli atlama tablosuyla dallaniyor;
 * her dal kaydin +0x34 alanina bir isleyici fonksiyon isaretcisi,
 * ortak kuyruk ise +0x38 alanina ROM'daki tanim blogunun adresini
 * yaziyor.  Tanim blogu bos degilse ilk halfword'u +0x04'e kopyalaniyor
 * (tanimin ilk alani muhtemelen grafik/kimlik numarasi).
 *
 * Turlerin cogu (2..26) ayni isleyiciyi (FUN_08018b74) paylasiyor ve
 * yalnizca tanim blogu adresiyle ayriliyor — bloklar 0x08CA45CC'den
 * baslayan bir ROM tablosunda 0x54 bayt araliklarla duruyor.
 * Tur 0 ve 1 ozel isleyicilere gidiyor; tur 27 ve aralik disi degerler
 * ortak "bos" isleyiciye (FUN_0801979c) ve NULL tanima dusuyor.
 *
 * ROM'daki blok sirasi kaynak sirasini birebir yansitiyor: 0..25,
 * sonra `case 27` + `default` ortak blogu, en sonda `case 26`.
 * Bu yuzden `default` switch'in ORTASINDA yazildi — sona alinirsa
 * blok sirasi kayiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/big_a4.c
 */

#include "gba_types.h"

/* Saklanan fonksiyon isaretcilerinde THUMB BITI kurulu olmali; `__thumb`
   sonekli sembol adresi | 1 olarak cozumlenir (tools/agbcc_build.py). */
extern u8 FUN_08017628__thumb[];
extern u8 FUN_080172a0__thumb[];
extern u8 FUN_08018b74__thumb[];
extern u8 FUN_0801979c__thumb[];

/* Tanim bloklari salt-okunur ROM verisi; her biri tek basina kullaniliyor,
   taban + ofset katlanmasi soz konusu degil (kural 1'in gerekcesi yok).
   data/ altina sembol EKLEME yetkim olmadigi icin sabit cast yazildilar. */
#define DEF(addr) ((const u16 *)(addr))

#define DEF_00  DEF(0x08CA45CC)
#define DEF_02  DEF(0x08CA4620)
#define DEF_03  DEF(0x08CA46C8)
#define DEF_04  DEF(0x08CA471C)
#define DEF_05  DEF(0x08CA4770)
#define DEF_06  DEF(0x08CA47C4)
#define DEF_07  DEF(0x08CA49BC)
#define DEF_08  DEF(0x08CA4A10)
#define DEF_09  DEF(0x08CA4914)
#define DEF_10  DEF(0x08CA4968)
#define DEF_11  DEF(0x08CA4A64)
#define DEF_12  DEF(0x08CA4818)
#define DEF_13  DEF(0x08CA486C)
#define DEF_14  DEF(0x08CA48C0)
#define DEF_15  DEF(0x08CA4AB8)
#define DEF_16  DEF(0x08CA4B0C)
#define DEF_17  DEF(0x08CA4B60)
#define DEF_18  DEF(0x08CA4BB4)
#define DEF_19  DEF(0x08CA4C08)
#define DEF_20  DEF(0x08CA4C5C)
#define DEF_21  DEF(0x08CA4CB0)
#define DEF_22  DEF(0x08CA4D04)
#define DEF_23  DEF(0x08CA4D58)
#define DEF_24  DEF(0x08CA4DAC)
#define DEF_25  DEF(0x08CA4E00)
#define DEF_26  DEF(0x08CA4674)

typedef struct Entity {
    u8         pad00[4];
    u16        unk04;           /* +0x04 tanimin ilk halfword'u */
    u8         pad06[0x2E];
    void      *handler;         /* +0x34 */
    const u16 *def;             /* +0x38 */
} Entity;

/* 0x0801528C */
void FUN_0801528c(Entity *entity, u32 kind)
{
    const u16 *def;
    const u16 *cur;

    switch (kind) {
    case 0:
        entity->handler = FUN_08017628__thumb;
        def = DEF_00;
        break;
    case 1:
        entity->handler = FUN_080172a0__thumb;
        def = 0;
        break;
    case 2:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_02;
        break;
    case 3:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_03;
        break;
    case 4:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_04;
        break;
    case 5:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_05;
        break;
    case 6:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_06;
        break;
    case 7:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_07;
        break;
    case 8:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_08;
        break;
    case 9:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_09;
        break;
    case 10:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_10;
        break;
    case 11:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_11;
        break;
    case 12:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_12;
        break;
    case 13:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_13;
        break;
    case 14:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_14;
        break;
    case 15:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_15;
        break;
    case 16:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_16;
        break;
    case 17:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_17;
        break;
    case 18:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_18;
        break;
    case 19:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_19;
        break;
    case 20:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_20;
        break;
    case 21:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_21;
        break;
    case 22:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_22;
        break;
    case 23:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_23;
        break;
    case 24:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_24;
        break;
    case 25:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_25;
        break;
    case 27:
    default:
        entity->handler = FUN_0801979c__thumb;
        def = 0;
        asm volatile ("");
        break;
    case 26:
        entity->handler = FUN_08018b74__thumb;
        def = DEF_26;
        break;
    }

    /* ROM alani yazdiktan SONRA yeniden okuyor (str, ldr, cmp). Kural 39:
       volatile yalnizca bu okumaya uygulaniyor; alanin tamamini volatile
       yapmak 28 dalin store'unu da etkilerdi. */
    entity->def = def;
    cur = *(const u16 *volatile *)&entity->def;
    if (cur != 0)
        entity->unk04 = *cur;
}
