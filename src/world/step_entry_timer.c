/* Giris zamanlayicisini ilerletme — 0x08028D44-0x08028DC3
 *
 * Indisten 148 baytlik giris hesaplayip iki asamali ROM tablosundan
 * hedefi cozuyor; giris uygun durumdaysa ve +0x90 alani 3 ise isaretci
 * kurup FUN_08013CFC'yi cagiriyor, zamanlayiciyi 4 azaltiyor ve pozitif
 * kalirsa 1 donuyor.
 *
 * PARAMETRE TIPLERI komut dizisinden okundu: ikinci parametre `lsls #16 /
 * asrs #16` ile ISARETLI 16 bit (s16), ucuncusu `lsls #24 / lsrs #24` ile
 * ISARETSIZ 8 bit (u8). Genis tip yazmak bu kirpma komutlarini goturur.
 *
 * Zamanlayici sinamasi `lsls #16` + `cmp <= 0`, yani azaltilmis yarim soz
 * ISARETLI olarak sinaniyor.
 *
 * HENUZ ESLESMIYOR: 2/128. BOYUT DOGRU. Kalan iki fark:
 *     +0x4C  bizim `lsrs r0,r0,#2`   ROM `asrs r0,r0,#2`  (isaretli kaydirma)
 *     +0x56  bizim `adds r0,r4,#0`   ROM `adds r0,r4,#4`  (isaretci +4)
 *
 * IKISI DE TEK BASINA DUZELTILEBILIYOR AMA HER BIRI 4 BAYTA MAL OLUYOR:
 *   offset alanini s32 yapmak            -> 124 bayt (asrs dogru, boyut yanlis)
 *   kaydirmada (s32) cast                -> 124 bayt
 *   ilk argumani (u8*)entry+4 yapmak     -> 124 bayt
 *   ilk argumani entry->pad04 yapmak     -> 124 bayt
 *   ilk argumani &entry->timer + 1       -> 124 bayt
 * Yani bu iki noktaya dokunmak baska bir yerde iki komut goturuyor;
 * muhtemelen struct yerlesimi tam dogru degil ve "dogal" ifade ROM'unkiyle
 * ayni bicime gelmiyor. Sonraki tur once yerlesimi dogrulamali.
 *
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/step_entry_timer.c
 */

#include "gba_types.h"

#define ENTRY_SIZE   148
#define STATE_READY  1
#define PHASE_DONE   3
#define TIMER_STEP   4
#define MARK_VALUE   0xFF

#define ROM_TABLE ((TableA *)0x08BD3448)

typedef struct TableB {
    u8    pad00[4];
    u32 **slots;                /* +0x04 */
} TableB;

typedef struct TableA {
    u8       pad00[4];
    TableB **slots;             /* +0x04 */
} TableA;

typedef struct Entry {
    u8  pad00[2];
    u16 timer;                  /* +0x02 */
    u8  pad04[38];
    u8  mark;                   /* +0x2A */
    u8  state;                  /* +0x2B */
    u8  pad2C[56];
    u8  tableIndex;             /* +0x64 */
    u8  pad65[39];
    u32 offset;                 /* +0x8C */
    u32 phase;                  /* +0x90 */
} Entry;

extern u8 gRam020246F0[];

extern void FUN_08013cfc(void *dest, u32 *src, u32 arg);

/* 0x08028D44 */
u32 StepEntryTimer(u32 unused, s16 index, u8 arg)
{
    Entry *entry;
    TableB *b;
    u32 *target;
    u32 phase;

    entry = (Entry *)(gRam020246F0 + index * ENTRY_SIZE);
    b = ROM_TABLE->slots[entry->tableIndex];
    phase = entry->phase;
    target = (u32 *)b->slots[phase];

    if (entry->state != STATE_READY)
        return 0;
    if (phase != PHASE_DONE)
        return 1;

    entry->mark = MARK_VALUE;
    FUN_08013cfc(&entry->timer - 1,
                 (u32 *)((TableB *)target)->slots[entry->offset >> 2], arg);

    entry->timer -= TIMER_STEP;
    if ((s16)entry->timer <= 0)
        return 0;
    return 1;
}
