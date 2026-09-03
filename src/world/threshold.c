/* Bant hesaplayici — 0x08023A0C-0x08023A67
 *
 * Bir "olcek" struct'inda current'i max ile sinirlayip 4 shift'lik bir
 * banda gore state'i (bant numarasi) yaziyor: current oranina gore
 * `{max>>0, max>>1, max>>3, max>>32}` esiklerinden hangisinin altinda
 * kaldigini state'e yaziyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/threshold.c
 */

#include "gba_types.h"

#define BAND_COUNT   4

typedef struct Meter {
    u8  state;                  /* +0x00 (bant no) */
    u8  flag;                   /* +0x01 */
    u8  pad02[2];
    s32 max;                    /* +0x04 */
    s32 current;                /* +0x08 */
} Meter;

#define gShiftTable ((const u32 *)0x08342AC8)

/* 0x08023A0C */
void ApplyThreshold(Meter *m, s32 value)
{
    s32 band;
    s32 max;
    s32 current;
    const u32 *shift;

    m->current = value;
    if (value > m->max)
        m->current = m->max;

    band = 0;
    max = m->max;
    current = m->current;
    shift = gShiftTable;

    do {
        if (current <= (max >> *shift))
            m->state = band;
        shift++;
        band++;
    } while (band <= BAND_COUNT - 1);
}

/* 0x08023A40 */
void ResetMeter(Meter *m)
{
    m->current = 0;
    m->state = 3;
}

/* 0x08023A4C */
void SetToMax(Meter *m)
{
    m->current = m->max;
    m->state = 0;
}

/* 0x08023A58 */
void ReloadMeter(Meter *m, s32 newMax)
{
    m->flag = 0;
    m->max = newMax;
    ApplyThreshold(m, newMax);
}
