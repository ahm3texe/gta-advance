/* Varlik ilklendirme — 0x080154D8-0x0801558F
 *
 * Bir varlik yapisini varsayilan degerlere kuruyor. 0x88'den sonraki bayt
 * alanlari 0xFF ile dolduruluyor (muhtemelen "atanmamis" isaretcisi);
 * 0x8A ve 0xA8'in dusuk dort biti temizleniyor.
 *
 * Thumb'da ldrb/strb yalnizca 0-31 ofseti alabildigi icin 0x88 ve sonrasina
 * erisirken derleyici yeni bir taban hesapliyor; ROM'daki `adds r1, #40`
 * zincirleri bundan.
 *
 * HENUZ ESLESMIYOR: 94 komutun 86'si birebir tutuyor, 14 bayt fark. Kalan
 * fark 0xA8 alanini maskeleyen taban isaretcisinin register'i:
 *     ROM  : taban r2, okunan bayt r3
 *     bizim: taban r3, okunan bayt r4
 * Cunku sifir sabiti bizde bir komut once maddelesip r2'yi kapiyor. Sifirin
 * dort referansi (0x90, 0x98, 0xA5, 0xAC) tabanin ucunden onde geliyor
 * (docs/COMPILER.md, register dagitiminin mekanizmasi).
 *
 * Cozulen: 0x8A ve 0xA8 alanlari s8 olmali. u8 iken derleyici maskeyi
 * 8 bite daraltip `mov r1,#0xF0` yaziyor; ROM ise 32 bitlik -16'yi
 * `mov r1,#0x10; neg r1,r1` ile kuruyor. Bu tek degisiklik 40 -> 14.
 *
 * Denenenler (hicbiri ilerletmedi): maskeyi -16 yazmak, yerel int maske,
 * tum slot alanlarini s8 yapmak (113), dongu sayacini int yapmak (78),
 * dort ayri dongu bicimi, alan tiplerini degistirmek.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_init.c
 */

#include "gba_types.h"

#define ACTOR_HEALTH_INIT   99
#define ACTOR_COORD_UNSET   0xFDFD
#define ACTOR_ANGLE_INIT    0x4000
#define ACTOR_RANGE_INIT    0x7FFF
#define SLOT_UNSET          0xFF
#define ACTOR_TASK_SLOTS    4

typedef struct Actor {
    u32 unk00;              /* +0x00 */
    u16 unk04;              /* +0x04 */
    u16 unk06;              /* +0x06 */
    u8  unk08;              /* +0x08 */
    u8  pad09[7];
    u32 unk10;              /* +0x10 */
    u32 unk14;              /* +0x14 */
    u8  pad18[4];
    u32 unk1C;              /* +0x1C */
    u16 unk20;              /* +0x20 */
    u16 unk22;              /* +0x22 */
    u32 unk24;              /* +0x24 */
    u32 unk28;              /* +0x28 */
    u8  pad2C[4];
    u32 unk30;              /* +0x30 */
    u32 unk34;              /* +0x34 */
    u32 unk38;              /* +0x38 */
    u32 unk3C;              /* +0x3C */
    u8  pad40[0x48];
    u8  slot88;             /* +0x88 */
    u8  pad89;
    s8  slot8A;             /* +0x8A */
    u8  slot8B;             /* +0x8B */
    u8  pad8C;
    u8  slot8D;             /* +0x8D */
    u8  pad8E[2];
    u32 unk90;              /* +0x90 */
    u8  pad94[4];
    u32 unk98;              /* +0x98 */
    u8  pad9C[4];
    u8  slotA0[4];          /* +0xA0 */
    u8  padA4;
    u8  unkA5;              /* +0xA5 */
    u8  padA6;
    u8  slotA7;             /* +0xA7 */
    s8  slotA8;             /* +0xA8 */
    u8  padA9[3];
    u32 unkAC;              /* +0xAC */
    u8  slotB0;             /* +0xB0 */
    u8  slotB1;             /* +0xB1 */
} Actor;

/* 0x080154D8 */
void InitActor(Actor *actor, u32 owner, u32 param)
{
    u8 i;

    actor->unk20 = ACTOR_COORD_UNSET;
    actor->unk22 = ACTOR_COORD_UNSET;
    actor->unk24 = 0;
    actor->unk00 = owner;
    actor->unk3C = 0;
    actor->unk34 = 0;
    actor->unk04 = 0;
    actor->unk06 = ACTOR_ANGLE_INIT;
    actor->unk10 = 0;
    actor->unk08 = ACTOR_HEALTH_INIT;
    actor->unk14 = 0;
    actor->unk1C = 0;
    actor->unk28 = ACTOR_RANGE_INIT;
    actor->unk30 = param;

    actor->slot88 |= SLOT_UNSET;
    actor->unk38 = 0;
    actor->slotB0 |= SLOT_UNSET;
    actor->slotA7 |= SLOT_UNSET;
    actor->slot8D |= SLOT_UNSET;

    for (i = 0; i < ACTOR_TASK_SLOTS; i++)
        actor->slotA0[i] |= SLOT_UNSET;

    actor->slotB1 |= SLOT_UNSET;
    actor->slot8B |= SLOT_UNSET;
    actor->slotA8 &= ~0x0F;

    actor->unk90 = 0;
    actor->unk98 = 0;
    actor->slot8A &= ~0x0F;
    actor->unkA5 = 0;
    actor->unkAC = 0;
}
