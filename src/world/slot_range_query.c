/* Yuva isareti, deger araligi ve ROM bayt tablosu — 0x08065518-0x08065573
 *
 * Ucu de A yuvasi cevresinde donuyor.  Birincisi varligin sahibini bulup
 * yuvanin +0x25 isaretini yaziyor, ikincisi A yuvasindaki +0x4C degerinin
 * ust yarim kelimesinin verilen aralikta olup olmadigini soyluyor,
 * ucuncusu 0x08F72620 ROM tablosundan tek bayt okuyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_range_query.c
 */

#include "gba_types.h"

#define SELECT_A 1

typedef struct PlayerSlot {
    u8  pad00[0x25];
    u8  marked;                 /* +0x25 */
    u8  pad26[0x4c - 0x26];
    u32 packed;                 /* +0x4C: ust yarim kelime konum degeri */
} PlayerSlot;

typedef struct Actor {
    u8    pad00[0x64];
    void *owner;                /* +0x64 */
} Actor;

extern u32       GetOwnerSlot(void *entity);
extern void     *SelectSlotAB(u32 which);
extern const u8  gRom08F72620[];

/* 0x08065518 */
void SetOwnerMark(Actor *actor, s32 value)
{
    PlayerSlot *slot;

    if (value != 0) {
        slot = (PlayerSlot *)SelectSlotAB(GetOwnerSlot(actor->owner));
        if (slot != 0)
            slot->marked = value;
    }
}

/* 0x08065538 */
s32 IsSlotValueInRange(s32 unused, u16 low, u16 high)
{
    PlayerSlot *slot;

    /* Erken cikisli zincir SART (kural 51): tek `&&` ifadesiyle yazilinca
     * low ve high pseudo'lari ayni omru (13) ve ayni onceligi paylasip
     * yazmaclari allocno sirasina gore aliyor ve r4/r5 ters dusuyor.
     * Bu bicim low'un omrunu 14'e cikarip onceligini dusuruyor, boylece
     * high once dagitiliyor ve ROM gibi r4'u aliyor. */
    slot = (PlayerSlot *)SelectSlotAB(SELECT_A);
    if (slot == 0)
        return 0;
    if (slot->packed == 0)
        return 0;
    if ((slot->packed >> 16) < low)
        return 0;
    if ((slot->packed >> 16) >= high)
        return 0;

    return 1;
}

/* 0x08065568 */
u8 LookupRomByte(s32 index)
{
    return gRom08F72620[index];
}
