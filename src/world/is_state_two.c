/* Durum == 2 sorgusu — 0x0805148C-0x0805149F
 *
 * 0x020303D0 blogunun +0x0C alanini 2 ile karsilastirip 1/0 donduruyor.
 * tools/find_predicates.py ile bulundu.
 *
 * ONCEDEN BLOKEYDI: adres haritada `gRecordIndex` adiyla ve `extern u32`
 * olarak kayitliydi, oysa ROM onu TABAN alip +0x0C okuyor. Cozum, sembolu
 * struct gorunumune cevirmek oldu: +0'daki indis `RecordBlock.index`,
 * +0x0C'deki durum `RecordBlock.state`. Sembol adi ve yerlesim degismedi,
 * bu yuzden src/misc/record_table.c bozulmadan kaldi.
 *
 * RecordBlock tanimi src/misc/record_table.c ile BIREBIR AYNI olmali.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/is_state_two.c
 */

#include "gba_types.h"

#define STATE_READY 2

typedef struct RecordBlock {
    u32 index;                  /* +0x00 */
    u8  pad04[8];
    u32 state;                  /* +0x0C */
} RecordBlock;

extern RecordBlock gRecordIndex;

/* 0x0805148C */
u32 IsStateReady(void)
{
    if (gRecordIndex.state == STATE_READY)
        return 1;
    return 0;
}
