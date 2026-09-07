/* Firlatma hizini uygulama — 0x08064DFC-0x08064F21 (294 bayt)
 *
 * DURUM: 119/143 komut, YAKIN ISKA (eslesmiyor). 292/294 bayt: ROM 2 bayt uzun.
 *
 * NE YAPIYOR: yuvaya kip baytini yazar (+0x25); hiz ve 16.16 acidan
 * (aci >>16) ileri/geri kirpma yapar (ileri: aci 1..0x2FF ise cik,
 * geri: aci > 0x100 ise cik); ileriyse aciyi 0x400'den cikarip
 * gRom08CA30D8 sinus tablosundan (<<2, >>8) hizin mutlak degeriyle
 * (>>8) carpip <<8 ile +0x28'e yazar; +0x40 = 0; hiz negatifse +0x124'u
 * ters cevirir; +0x3C = +0x124'un yarisi (isaret ters); |+0x3C| <
 * (+0xE0 - +0xD4)/4 ise sifirlar; +0x18C..+0x19C zincirinden secim yapip
 * +0x120'ye ekler; hizi ScaleByDistanceBand ile olcekler; ses 472.
 *
 * KALAN 24 KOMUT, UC KUME (hepsi yazmac atamasi / okuma sirasi):
 *  1. sinus r0 / mutlak-hiz r1 rolleri ters (bende r1 / r0).
 *  2. +0x3C ternary'si ROM'da dogrudan r0'a hesaplanip saklaniyor ve
 *     karsilastirma icin alan YENIDEN OKUNUYOR (`ldr r1,[r4,#60]`);
 *     bende deger r2'de kaliyor. Denenen: yerel v, dogrudan ternary,
 *     `*w` takma ad isaretcisi (26), d'yi once hesaplamak -- hicbiri.
 *  3. +0x18C/+0x190/+0x194 uclusunun adres sabitleri: ROM +0x194'u
 *     +0x18C sabitinden (+8) turetiyor, bende +0x190'dan (+4). Denenen:
 *     ayri yereller p0..p4 (118), `q = &b->unk18C; q[i]` (98),
 *     kaydirmalari satir ici yapmak (119, en iyi).
 * Bu uc kume kural 44 sinifi (yazmac atamasi); bilinen kaynak kaldiraci
 * yok. Yeni deneyen: docs/COMPILER.md kural 51/54'e bakip once 2'yi denesin.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/apply_launch_velocity.c
 */

#include "gba_types.h"
#define ANGLE_MASK  0x3FF
#define FULL_TURN   0x400
#define FWD_MAX     0x2FF
#define BACK_MAX    0x100
#define SOUND_ID    472
typedef struct Inner { u8 pad00[0x38]; s32 band; } Inner;
typedef struct Link { u8 pad00[16]; Inner *inner; } Link;
typedef struct Body {
    u8 pad00[0x18]; s32 speed; u8 pad1c[0xC]; s32 unk28; u8 pad2c[0x10];
    s32 unk3C; s32 unk40; s32 angle; u8 pad48[0x1C]; void *entity; u8 pad68[0x6C];
    s32 unkD4; u8 padd8[8]; s32 unkE0; u8 pade4[0x3C]; s32 unk120; s32 unk124;
    u8 pad128[0x64]; s32 unk18C; s32 unk190; s32 unk194; s32 unk198; s32 unk19C;
    u8 pad1a0[0x10]; Link *link;
} Body;
extern const s16 gRom08CA30D8[];
extern u32   GetOwnerSlot(void *entity);
extern u8   *SelectSlotAB(u32 which);
extern s32   ScaleByDistanceBand(s32 value, s32 band);
extern void  FUN_08035168(u32 id);
void ApplyLaunchVelocity(Body *b, u32 mode)
{
    u8 *slot; s32 speed; s32 sp; s32 ang; s32 sine; s32 mag; s32 d; s32 sel; s32 p0; s32 p1; s32 p2; s32 p3; s32 p4;
    slot = SelectSlotAB(GetOwnerSlot(b->entity));
    if (slot != 0 && mode != 0)
        slot[0x25] = mode;
    speed = b->speed;
    ang = b->angle >> 16;
    sp = speed;
    if (sp > 0 && ang <= FWD_MAX && ang > 0)
        return;
    if (speed < 0 && ang > BACK_MAX)
        return;
    if (speed > 0)
        ang = FULL_TURN - ang;
    sine = gRom08CA30D8[ang & ANGLE_MASK] << 2;
    mag = speed;
    if (mag < 0)
        mag = -mag;
    b->unk28 = ((sine >> 8) * (mag >> 8)) << 8;
    b->unk40 = 0;
    if (sp < 0)
        b->unk124 = -b->unk124;
    b->unk3C = (b->unk124 < 0) ? (-b->unk124 >> 1) : -(b->unk124 >> 1);
    d = (b->unkE0 - b->unkD4) >> 2;
    if (b->unk3C > -d && b->unk3C < d)
        b->unk3C = 0;
    p0 = b->unk18C;
    p1 = b->unk190;
    p2 = b->unk194;
    sel = p0;
    if (p2 > sel) sel = p1;
    p3 = b->unk198;
    if (p3 > sel) sel = p2;
    p4 = b->unk19C;
    if (p4 > sel) sel = p3;
    b->unk120 += sel;
    b->speed = ScaleByDistanceBand(b->speed, b->link->inner->band);
    FUN_08035168(SOUND_ID);
}
