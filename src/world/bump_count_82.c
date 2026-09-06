/* Gecikmeyle cevrelenmis kayit sayaci — 0x08067184-0x080671B7
 *
 * stat_counters.c'deki tasma korumali artirimin aynisi, bu kez kayit
 * tamponunun +0x82 alaninda ve iki bos dongu gecikmesinin arasinda.
 * SpinDelay govdesinde argumani KULLANMIYOR (0x08030EA0 byte-matching,
 * src/world/spin_delay.c), ama cagri yerinde r0'a sabit yukleniyor;
 * bu yuzden burada argumanli bildirim sart.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/bump_count_82.c
 */

#include "gba_types.h"

typedef struct SaveCounters82 {
    u8  pad00[0x82];
    u16 count82;                /* +0x82 */
} SaveCounters82;

extern SaveCounters82 gSaveBuffer;
extern void SpinDelay(s32 tag);

/* 0x08067184 */
void BumpCount82(void)
{
    u16 old;
    int next;

    SpinDelay(401);

    old = gSaveBuffer.count82;
    next = old + 1;
    gSaveBuffer.count82 = next;
    if ((u16)next == 0)
        gSaveBuffer.count82 = old;

    SpinDelay(403);
}
