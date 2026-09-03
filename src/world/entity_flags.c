/* Varlik bayrak sorgusu — 0x08032058-0x0803208F
 *
 * BYTE-MATCHING.  Govde 56 bayt: 23 komut + 2 bayt hizalama dolgusu + 4 bayt
 * literal havuzu + 2 komut.  data/functions.csv bu satira 50 bayt yaziyor;
 * gercek govde bir sonraki fonksiyona (GetEntityFixedFields, 0x08032090)
 * kadar suruyor.
 *
 * Eslesmeyi saglayan iki ayrinti — ikisi de olculdu, ikisi de gerekli:
 *
 * 1. Bayrak sozcukleri tamponun 60. baytindan baslar ve ROM tabani AYRI
 *    yukleyip 60 EKLIYOR:  ldr r0,=gSaveBuffer / adds r0,#60 / adds r2,r2,r0
 *    Bu bicimi yalnizca STRUCT UYESI erisimi uretiyor.  `&gSaveBuffer[60]` ve
 *    `(u8 *)gSaveBuffer + 60` yazimlarinda agbcc ofseti literal havuzuna
 *    katliyor (`.word gSaveBuffer+0x3c`); yerel taban isaretcisi ise ofseti
 *    yukleme komutuna gomuyor (`ldr r0,[r0,#0x3c]`).  Ucu de 52 bayt uretir.
 *
 * 2. Ham id ile son indeks AYNI degiskende tutulmali.  Ayri degiskenlerle
 *    agbcc `id` ile `id - 1`'i tek register'da birlestirip `subs r0,#1`
 *    uretiyor; ROM'da `subs r0,r3,#1` var.  Tek degiskende `index`'in omru
 *    cikarma komutunu kapsiyor, iki miktar catisiyor ve register dagitimi
 *    ROM'unkine oturuyor: id/indeks r3'te, ara deger r0'da.
 *
 * Ayrica indeks kaydirmasi ISARETLI (ROM `asrs`), sinir karsilastirmasi
 * ISARETSIZ (ROM `bhi`) olmali.
 *
 * Denenip tutmayanlar (hepsi struct uyesi bicimiyle olculdu):
 *   &gSaveBuffer[60] .......................... 52 bayt / 39 fark
 *   yerel u8 * taban + 60 ..................... 52 bayt / 39 fark
 *   extern u32 dizi + gSaveBuffer[word + 15] .. 56 bayt / 17 fark
 *   word/bit yerine tek satirlik ifade ........ 56 bayt / 21 fark
 *   ayri `mask` yereli ........................ 56 bayt / 22 fark
 *   `return (...) != 0;` ...................... 56 bayt / 47 fark
 *   ayri `id` yereli (u16/int/u32) ............ 56 bayt /  8 fark
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entity_flags.c
 */

#include "gba_types.h"

typedef struct {
    u8  unk00[22];
    u16 id;                 /* 0x16 — bir tabanli; 0 "yok" demek        */
} Entity;

/* Kayit slotu calisma tamponu.  Baslik ve tumleyen alanlari save_manager.c
 * uzerinden biliniyor; aradaki bloklarin icerigi henuz cozulmedi.  Varlik
 * bayraklari 60. bayttan baslayan dort sozcukte duruyor. */
typedef struct {
    u8  header[12];         /* 0x00 — marker + slot basligi              */
    u8  unk0C[48];          /* 0x0C — cozulmedi                          */
    u32 entityFlags[4];     /* 0x3C — varlik basina bir bit, 128 bit     */
    u8  unk4C[80];          /* 0x4C — cozulmedi                          */
    u8  complement;         /* 0x9C — marker'in tumleyeni                */
    u8  unk9D[3];           /* 0x9D — cozulmedi                          */
} SaveBuffer;

#define ENTITY_ID_MAX 128

extern SaveBuffer gSaveBuffer;

/* 0x08032058 */
u32 IsEntityFlagSet(const Entity *entity)
{
    s32 index, word, bit;

    /* Ham id ve indeks bilerek tek degiskende; bkz. dosya basi, 2. madde. */
    index = entity->id;
    if (index == 0)
        return 0;

    index = (u16)(index - 1);
    if ((u32)index > ENTITY_ID_MAX - 1)
        return 0;

    word = index >> 5;
    bit = index & 31;
    if (gSaveBuffer.entityFlags[word] & (1 << bit))
        return 1;

    return 0;
}
