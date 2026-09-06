/* Varliktan konum+aciyi aktore kopyalayip iki kayit arasindaki farki
 * dondurerek aktorun 16.16 konumuna ekler -- 0x080293F8-0x080294B1,
 * 186 bayt, Thumb.  ESLESIYOR (186/186).
 *
 * ISKELET (ROM'dan okundu, dort parametre: r0 varlik, r1 aktor, r2/r3 kayit)
 *
 *   1. entity->source (+0x18) uc kelimelik konumu aktorun +0x4C'sine
 *      KOPYALANIYOR (ldmia/stmia {r2,r4,r5}), ardindan +0x0C'deki aci
 *      aktorun +0x68'ine yaziliyor.
 *   2. Her iki Record icin orta nokta: (x0 + x1) - ox.  DIKKAT: kardeslerdeki
 *      `-2` ofseti ve `<<23 >>24` isaret genisletmesi BURADA YOK; ham fark
 *      kullaniliyor.  Kardesin bicimini kopyalamamak gerektiginin ornegi.
 *   3. Iki orta nokta farki (3. parametre eksi 4. parametre) FUN_0802915c'ye
 *      gidiyor.  Bu, entries_c1/entries_b4'teki FUN_08029088 DEGIL: yigina
 *      s8 degil TAM KELIME yaziyor (sonuclar `ldr` ile okunuyor), yigin
 *      gozleri de sp+4 / sp+8, yani bitisik degil ayri iki kelime.
 *      Imzasi 0x0802915C'den okundu: (aci, dx, dy, s32 *outX, s32 *outY),
 *      govdesi 0x08CA30D8'deki sinus tablosuyla dondurme yapiyor.
 *   4. Donen iki deger 328/256 ile olceklenip << 15 ile +0x4C/+0x50'ye
 *      EKLENIYOR (kopyalanan konumun ustune).
 *   5. Kuyruk, state_offset.c'nin VARSAYILAN dalinin birebir aynisi:
 *      nested->facing baytlari okunup aktorun +4 alt yapisina sifir
 *      denetimiyle yaziliyor.
 *
 * ESLESMEYI SAGLAYAN UC OLCUM (184 kisa -> 24 fark -> 8 fark -> 0)
 *
 *  1. ACI YERELE ALINIR.  `FUN_0802915c(src->angle >> 16, ...)` yazildiginda
 *     agbcc kaydirmayi yutuyor ve `movs r5,#14; ldrsh r0,[r3,r5]` uretiyor --
 *     yani 16.16 kelimenin ust yarisini DOGRUDAN isaretli yarim soz olarak
 *     okuyor; src bu yuzden cagriya kadar canli kaliyor ve IKINCI bir yuksek
 *     yazmac (r9) aciliyor, prolog/epilog sisiyor.  `angle = src->angle;`
 *     yereli hem `asrs r0,r0,#16` uretiyor hem de src'yi erken olduruyor:
 *     ROM'un tek yuksek yazmaci (r8 varlik, ip gecici) geri geliyor.
 *     KURAL: dar okuma optimizasyonunu istemiyorsan degeri yerele al.
 *
 *  2. FARKLAR AYRI DEYIM OLUR.  Cikarmalar cagri argumaninda yazilinca
 *     agbcc once 1. argumanin kaydirmasini, sonra cikarmalari yapiyor;
 *     ROM'un sirasi tersi (once iki cikarma, sonra `asrs #16`).  `dx`/`dy`
 *     yerelleri acmak kaydirmayi cagri yerine tasiyor: 8 -> 0 bayt.
 *     Bu, entries_c1'deki 1 numarali olcumun TERSI yonu; orada kaydirma
 *     argumanda kalmaliydi.  Kaynagi ROM'un sirasi belirliyor, kardes degil.
 *
 *  3. YONELIM YERELLERI s8 DEGIL s32.  `s8 fx` ile agbcc iki bayti de duz
 *     `ldrb` ile okuyup isaret genisletmiyor (depolama zaten strb).  ROM ise
 *     ikisini de isaretli okuyor: birinci icin `ldrb + lsls#24 + asrs#24`,
 *     ikinci icin `movs r2,#0; ldrsb r2,[r0,r2]`.  Asimetri kaynakta degil,
 *     adres kurulumunun yan etkisi (LDRSB'nin immediate ofseti yok, birinci
 *     okumada +34 adresi ayri yazmaca kuruldugu icin sifir yazmaci yok).
 *     s32 yerel her ikisini de isaretli okumaya cevirip farki kapatiyor.
 *
 * DENENIP ELENEN YAZIMLAR (silme, ekle)
 *
 *  - `FUN_0802915c(src->angle >> 16, ...)`: 184 bayt, ROM'dan 2 KISA.
 *    Yukaridaki 1 numarali olcum; `ldrsh` kisayolu yuzunden.
 *  - `s8 fx, fy;` yerelleri: iki isaret genisletme komutu dusuyor.
 *  - Cikarmalari arguman icinde birakmak: 8 bayt fark, komut sirasi kayiyor.
 *  - Kayit sirasini ters yazmak DENENMEDI, gerek kalmadi: ROM once
 *    4. parametreyi (r3) isliyor, kaynak da oyle yazildi.
 *
 * OLCULEN AYRINTILAR
 *
 *  - Toplamalarda agbcc IKINCI operandi ONCE yukluyor (entries_a3/c1 ile
 *    ayni): ROM `ldrb [r3,#6]` (x1) ile basliyor -> kaynakta `x0 + x1`.
 *  - Olcek carpani 41 DEGIL 328: lsls#2/adds/lsls#3/adds/lsls#3 zinciri
 *    41*8 uretiyor, ardindan `asrs #8`.  Kardeslerdeki 41/32 orani ayni,
 *    ama kaynaga 328/256 yazilmali; `41 >> 5` yazmak son lsls#3'u dusurur.
 *  - Uc kelimelik konum kopyasi struct atamasiyla (`actor->pos = src->pos;`)
 *    uretiliyor; agbcc 12 bayti ldmia/stmia ciftine ceviriyor.
 *  - Donus tipi void (kural 35): epilog `pop {r0}; bx r0`.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_c2.c   -> 186/186
 */

#include "gba_types.h"

/* 41*8 = 328.  ROM lsls#2/adds/lsls#3/adds/lsls#3/asrs#8 uretiyor, yani
 * carpan 41 DEGIL 328; kardeslerdeki 41/32 olcegi burada 8 ile buyutulmus. */
#define SCALE_NUM  328
#define SCALE_SH   8
#define POS_SH     15

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* state_offset.c'deki gorunumlerin aynisi. */
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

typedef struct Source {
    Vec3 pos;             /* +0x00..+0x0B */
    s32  angle;           /* +0x0C */
} Source;

typedef struct Entity {
    u8 pad0[0x18];
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
    u8   pad0[0x4c];
    Vec3 pos;             /* +0x4C..+0x57 */
    u8   pad58[0x10];
    s32  angle;           /* +0x68 */
} Actor;

extern void FUN_0802915c(s32 angle, s32 dx, s32 dy, s32 *outX, s32 *outY);

/* 0x080293F8 */
void FUN_080293f8(Entity *entity, Actor *actor, Record *recA, Record *recB)
{
    Source *src;
    Facing *facing;
    Slot *dst;
    s32 ax, ay, bx, by;
    s32 dx, dy;
    s32 ox, oy;
    s32 angle;
    s32 fx, fy;

    src = entity->source;
    actor->pos = src->pos;
    angle = src->angle;
    actor->angle = angle;

    bx = (recB->x0 + recB->x1) - recB->ox;
    by = (recB->y0 + recB->y1) - recB->oy;
    ax = (recA->x0 + recA->x1) - recA->ox;
    ay = (recA->y0 + recA->y1) - recA->oy;

    dx = ax - bx;
    dy = ay - by;
    FUN_0802915c(angle >> 16, dx, dy, &ox, &oy);

    ox = (ox * SCALE_NUM) >> SCALE_SH;
    oy = (oy * SCALE_NUM) >> SCALE_SH;
    actor->pos.x += ox << POS_SH;
    actor->pos.y += oy << POS_SH;

    facing = entity->nested->facing;
    fx = facing->fx;
    fy = facing->fy;
    dst = (Slot *)((u8 *)actor + 4);
    if (dst) {
        dst->fx = fx;
        dst->fy = fy;
    }
}
