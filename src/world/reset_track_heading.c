/* Iz baslangic acisini kurma — 0x0800AAF4-0x0800AB2F
 *
 * gRam02011030'un +0x3C/+0x40/+0x44/+0x10'unu sifirlayip +0x04'teki
 * dugumun acisini +0x0C'ye (<<16) yaziyor: +0x08 bayraginda 0x30
 * kuruluysa +0x20'deki kaydin +0x1F baytindaki 2 BITLIK alan <<8,
 * degilse +0x18'deki kaydin +0x0E acisi & 0x3FF.
 *
 * UC OLCUM: 2 bitlik alan BITFIELD olarak bildirilmeli (`u8 quad : 2`);
 * `(x & 3) << 8` ands/lsls uretiyor, ROM lsls#30/lsrs#22 (kural 61).
 * Yerel bir `t = &g` isaretcisi r1/r2 dagitimini ters ceviriyor; erisim
 * makro uzerinden (`TRACK->`) olmali. Sembol alti eslesen dosyayla
 * paylasilan CoordBlock govdesiyle bildirilmeli (tutarlilik kapisi);
 * bu fonksiyonun alanlari o govdede dolgu icinde kaldigi icin Track
 * gorunumu makroyla ustune bindiriliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/reset_track_heading.c
 */

#include "gba_types.h"
#define POS_FLAG   0x30
#define ANGLE_MASK 0x3FF
typedef struct Alt { u8 pad00[31]; u8 quad : 2; } Alt;
typedef struct Pos { u8 pad00[14]; u16 angle; } Pos;
typedef struct Node { u8 pad00[8]; u8 flags; u8 pad09[15]; Pos *pos; u8 pad1c[4]; Alt *alt; } Node;
typedef struct CoordBlock {
    u32 unk00;              /* +0  */
    u32 unk04;              /* +4  */
    u32 unk08;              /* +8  */
    s32 second;             /* +12 */
    s32 first;              /* +16 */
    u8  pad14[28];
    u32 a;                      /* +0x30 */
    u32 b;                      /* +0x34 */
    u32 c;                      /* +0x38 */
    u8  pad3C[12];
    u32 unk48;              /* +72 */
    u8  pad4C[37];
    u8  byte71;             /* +0x71 */
} CoordBlock;
extern CoordBlock gRam02011030;
typedef struct Track { u8 pad00[4]; Node *node; u8 pad08[4]; u32 heading; u32 unk10; u8 pad14[40]; u32 unk3C; u32 unk40; u32 unk44; } Track;
#define TRACK ((Track *)&gRam02011030)
u32 ResetTrackHeading(void)
{
    Node *node; u32 angle;
    TRACK->unk3C = 0;
    TRACK->unk40 = 0;
    TRACK->unk44 = 0;
    TRACK->unk10 = 0;
    node = TRACK->node;
    if (POS_FLAG & node->flags)
        angle = node->alt->quad << 8;
    else
        angle = ANGLE_MASK & node->pos->angle;
    TRACK->heading = angle << 16;
    return 1;
}
