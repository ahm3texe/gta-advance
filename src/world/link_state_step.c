/* Sirali sayac artirimi + uyari bayragi — 0x08066B40-0x08066C93
 *
 * bump_rank_counter.c'nin (0x08066C94) ikizi: ayni GetRecordIndex
 * anahtari, ayni "artir, 20'de doyur" kalibi. Fark, bu sayaclarin kayit
 * tamponunun +0x70'indeki tek u32 icinde durmasi ve her sayac icin iki
 * esik sinamasi (19 ve 9) bulunmasi — esiklerden birine denk gelirse
 * gRam02025810[0x137D] uyari bayragi 1 yapiliyor.
 *
 * Alanlarin bit yerlesimi agbcc'nin urettigi komutu belirliyor:
 *   bit 8-12  -> tek bayt  (ldrb/strb +0x71, maske 0x1F)
 *   bit 13-17 -> tam soz   (ldr/str  +0x70, bayt sinirini asiyor)
 *   bit 18-22 -> tek bayt  (ldrb/strb +0x72, maske 0x7C)
 *
 * ARTIRMA/DOYURMA bitfield yaziliyor; ESIK SINAMALARI ise kaydirmasiz
 * maske karsilastirmasi (bkz. ROM: "ands r0,#0x7C / cmp r0,#0x4C").
 * agbcc bitfield esitlik sinamasini maskeye cevirmiyor — alan int'e
 * yukseltildigi icin fold'un optimize_bit_field_compare yolu kapali,
 * `x.f == 19` her zaman lsl/lsr ile ayikliyor. Bu yuzden sinamalar
 * birlesimdeki ham gorunum (bytes[] / word) uzerinden yaziliyor;
 * ikisi ayni adresi gordugu icin agbcc taban adresini de tek yazmacta
 * paylasiyor, ROM'daki gibi.
 *
 * Doyurma karsilastirmasi ISARETSIZ (ROM: bls). 5 bitlik alan int'e
 * yukseldigi icin duz `> 20` isaretli `ble` uretir; sabit 20U yazildi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/link_state_step.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define COUNTER_CAP  20
#define WARN_FLAG    gRam02025810[0x137D]

/* Esikler ham (kaydirilmis) bicimde: 19 ve 9 */
#define A_MASK       0x1F
#define A_HIGH       (19 << 0)
#define A_LOW        ( 9 << 0)

#define B_MASK       0x3E000
#define B_HIGH       (19 << 13)
#define B_LOW        ( 9 << 13)

#define C_MASK       0x7C
#define C_HIGH       (19 << 2)
#define C_LOW        ( 9 << 2)

typedef struct StepCounters {
    u8 pad00[0x70];
    union {
        struct {
            u32 spare  : 8;     /* bit 0-7   */
            u32 countA : 5;     /* bit 8-12  */
            u32 countB : 5;     /* bit 13-17 */
            u32 countC : 5;     /* bit 18-22 */
        } f;
        u32 word;
        u8  bytes[4];
    } u;
} StepCounters;

extern StepCounters gSaveBuffer;
extern s32 GetRecordIndex(void);

/* 0x08066B40 */
void BumpStepCounter(void)
{
    switch (GetRecordIndex()) {
    case 0:
        if ((gSaveBuffer.u.bytes[1] & A_MASK) == A_HIGH)
            WARN_FLAG = 1;
        if ((gSaveBuffer.u.bytes[1] & A_MASK) == A_LOW)
            WARN_FLAG = 1;
        gSaveBuffer.u.f.countA++;
        if (gSaveBuffer.u.f.countA > (u32)COUNTER_CAP)
            gSaveBuffer.u.f.countA = COUNTER_CAP;
        break;
    case 1:
        if ((gSaveBuffer.u.word & B_MASK) == B_HIGH)
            WARN_FLAG = 1;
        if ((gSaveBuffer.u.word & B_MASK) == B_LOW)
            WARN_FLAG = 1;
        gSaveBuffer.u.f.countB++;
        if (gSaveBuffer.u.f.countB > (u32)COUNTER_CAP)
            gSaveBuffer.u.f.countB = COUNTER_CAP;
        break;
    case 2:
        if ((gSaveBuffer.u.bytes[2] & C_MASK) == C_HIGH)
            WARN_FLAG = 1;
        if ((gSaveBuffer.u.bytes[2] & C_MASK) == C_LOW)
            WARN_FLAG = 1;
        gSaveBuffer.u.f.countC++;
        if (gSaveBuffer.u.f.countC > (u32)COUNTER_CAP)
            gSaveBuffer.u.f.countC = COUNTER_CAP;
        break;
    }
}
