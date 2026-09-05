/* Duruma gore aktorun yonelim baytlarini ve 16.16 konum kaymasini hesaplar.
 *
 * 0x080260A8, 1518 bayt.  Aktorun +0x90'daki DURUM degerine gore dallaniyor.
 * Her dalda ayni iskelet var:
 *
 *   1. Varliktan bir ACI okunur, 0x03FFFFFF ile maskelenir (26 bit) ve
 *      aktorun +0x68 alanina yazilir.  Aci >> 16 ile 10 bitlik indekse
 *      donusuyor; FUN_08029088 bunu bir donusturme yardimcisi gibi aliyor.
 *   2. Kayittan iki eksen icin orta nokta hesaplanir:
 *      (b6 + b4) - (b18 - 2), sonra `<< 23 >> 24` ile yarilanip 9 bitten
 *      isaret genisletilir.
 *   3. Donen iki bayt 41/32 ile olceklenip +0x26 ve +0x27'ye yazilir.
 *   4. Ikinci bir cagri sonucu 16.16 olarak +0x4C ve +0x50'ye EKLENIR.
 *
 * Dallar arasindaki tek fark: aciya 0x2000000 eklenip eklenmedigi, ikinci
 * cagriya giden ikinci deger ve sonda yazilan kip baytı.
 *
 * DURUM: 1430/1518, 88 bayt KISA.  Dagitim zinciri ROM ile BIREBIR ayni
 * (25, 35, 33, 8, 30, 31, 40, 50, 32, 7, 6, 47 sirasiyla), sondaki ortak
 * epilog ve varsayilan dal da oturdu.  Kalan fark hala blok birlesmesi:
 * ROM'da 0x03FFFFFF maskesi ALTI ayri havuz kelimesinde duruyor, yani alti
 * fiziksel blok var; bizde bes.
 *
 * KAZANIM 1 -- 7 ve 6 AYRI dallar.  Ikisini `state == 7 || state == 6`
 * diye birlestirmistim; ROM'da ayri iki kopya.  Ayirmak 1172 -> 1312.
 *
 * KAZANIM 2 -- HER DALIN KENDI YERELLERI VAR.  Bu belirleyiciydi.  Once
 * tum dallara ortak `bias`/`shift` vermistim; govdeler birebir ayni olunca
 * agbcc case 8'in blogunu tamamen 0x1f/0x28 ile birlestirdi (bizde `cmp #8`
 * ile `cmp #30` arasi 4 bayt, ROM'da 156).  ROM'un yigin gozleri dallarin
 * ayri yereller kullandigini soyluyor: case 8 sp+8/12/16, case 0x1f
 * sp+32/36/40.  Dal basina ucer yerel acmak 1312 -> 1428.
 *
 * KAZANIM 3 -- varsayilan dal.  ROM `adds r0, r7, #4` ile aktorun +4
 * adresini kurup sifira karsi siniyor, sonra oradan +34/+35'e yaziyor.
 * Ghidra bunu anlamsiz gorunen `param_1 == -4` diye gosteriyordu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/state_offset.c
 */

#include "gba_types.h"

#define ANGLE_MASK 0x03FFFFFF
#define ANGLE_BIAS 0x02000000
#define SCALE_NUM  41
#define SCALE_SH   5

typedef struct Owner {
    u8 pad0[9];
    u8 kind;              /* +0x09 */
} Owner;

/* Aktorun +0x04'unden baslayan alt yapi; yonelim baytlari onun icinde
 * +34 ve +35'te.  ROM once `adds r0, r7, #4` ile bu adresi kuruyor ve
 * sifira karsi siniyor -- Ghidra'nin `param_1 == -4` diye gosterdigi sey
 * tam olarak budur. */
typedef struct Slot {
    u8 pad0[34];
    s8 fx;                /* +0x22 (aktorda +0x26) */
    s8 fy;                /* +0x23 (aktorda +0x27) */
} Slot;

typedef struct Facing {
    u8 pad0[0x22];
    s8 fx;                /* +0x22 */
    s8 fy;                /* +0x23 */
} Facing;

typedef struct Nested {
    u8 pad0[0x3c];
    Facing *facing;       /* +0x3C */
} Nested;

typedef struct Bounds {
    u8 pad0[0x4b];
    s8 drop;              /* +0x4B */
} Bounds;

typedef struct Source {
    u8 pad0[0x0c];
    s32 angle;            /* +0x0C */
} Source;

typedef struct Entity {
    u8 pad0[0x10];
    Bounds *bounds;       /* +0x10 */
    u8 pad14[4];
    Source *source;       /* +0x18 */
    Nested *nested;       /* +0x1C */
} Entity;

typedef struct Record {
    u8 pad0[4];
    u8 x0;                /* +0x04 */
    u8 y0;                /* +0x05 */
    u8 x1;                /* +0x06 */
    u8 y1;                /* +0x07 */
    u8 pad8[0x10];
    u8 ox;                /* +0x18 */
    u8 oy;                /* +0x19 */
} Record;

typedef struct Actor {
    u8 pad0[0x26];
    s8 fx;                /* +0x26 */
    s8 fy;                /* +0x27 */
    u8 pad28[2];
    u8 mode;              /* +0x2A */
    u8 pad2b[0x21];
    s32 px;               /* +0x4C */
    s32 py;               /* +0x50 */
    u8 pad54[0x14];
    s32 angle;            /* +0x68 */
    u8 pad6c[0x18];
    Owner *owner;         /* +0x84 */
    u8 pad88[8];
    s32 state;            /* +0x90 */
} Actor;

extern void FUN_08029088(s32 angle, s32 dx, s32 dy, s8 *outX, s8 *outY);
extern s32 GetNegatedNested(Entity *entity);

void FUN_080260a8(Actor *actor, Entity *entity, Record *rec)
{
    s32 bias;
    s32 state;
    s32 angle;
    s32 dx;
    s32 dy;
    s32 nested;
    s32 shift;
    s32 a08, b08, c08;
    s32 a1e, b1e, c1e;
    s32 a1f, b1f, c1f;
    s32 a28, b28, c28;
    s8 ox;
    s8 oy;

    bias = 0;
    if (actor->owner->kind == 25) bias = -28;

    state = actor->state;
    if (state == 0x23) {
        angle = entity->source->angle & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, -44, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x21;
        return;
    }
    if (state == 0x21) {
        angle = entity->source->angle & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, -44, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x21;
        return;
    }

    if (state == 8) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a08 = nested - (bias - 12);
        angle = actor->angle >> 16;
        b08 = a08 << 24;
        c08 = b08 >> 24;
        shift = c08;
    } else if (state == 0x1e) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a1e = nested - (bias - 16);
        angle = actor->angle >> 16;
        b1e = a1e << 24;
        c1e = b1e >> 24;
        shift = c1e;
    } else if (state == 0x1f) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a1f = nested - (bias - 12);
        angle = actor->angle >> 16;
        b1f = a1f << 24;
        c1f = b1f >> 24;
        shift = c1f;
    } else if (state == 0x28 || state == 0x32) {
        nested = GetNegatedNested(entity);
        angle = (entity->source->angle + ANGLE_BIAS) & ANGLE_MASK;
        actor->angle = angle;
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        a28 = nested - (bias - 12);
        angle = actor->angle >> 16;
        b28 = a28 << 24;
        c28 = b28 >> 24;
        shift = c28;
    } else if (state == 0x20) {
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(actor->angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, entity->bounds->drop, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x40;
        return;
    } else if (state == 7) {
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(actor->angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, bias, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x10;
        return;
    } else if (state == 6) {
        dx = (s32)((((rec->x1 + rec->x0) - (rec->ox - 2))) << 23) >> 24;
        dy = (s32)((((rec->y1 + rec->y0) - (rec->oy - 2))) << 23) >> 24;
        FUN_08029088(actor->angle >> 16, dx, dy, &ox, &oy);
        actor->fx = (ox * SCALE_NUM) >> SCALE_SH;
        actor->fy = (oy * SCALE_NUM) >> SCALE_SH;
        FUN_08029088(actor->angle >> 16, 0, bias, &ox, &oy);
        actor->px += ox << 16;
        actor->py += oy << 16;
        actor->mode = 0x10;
        return;
    } else if (actor->state == 0x2f) {
        actor->mode = 0x10;
        return;
    } else {
        Facing *facing = entity->nested->facing;
        Slot *dst;
        s8 fx;
        s8 fy;

        fx = facing->fx;
        fy = facing->fy;
        dst = (Slot *)((u8 *)actor + 4);
        if (dst) {
            dst->fx = fx;
            dst->fy = fy;
        }
        return;
    }

    FUN_08029088(angle, 0, shift, &ox, &oy);
    actor->px += ox << 16;
    actor->py += oy << 16;
}
