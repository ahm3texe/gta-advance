/* Aktorun durum degerine gore bir kare ilerletir.  0x08017628, 1536 bayt.
 *
 * Iskelet: +0x28'deki DURUM 0x7FFF (bos) degilse, 0x3FFF ustu degerler bir
 * yeniden esleme tablosundan gecirilir, sonra duruma gore buyuk bir switch
 * calisir ve durum 0x7FFF'e geri alinir.  Ardindan ortak bir kuyruk var.
 *
 * gcc switch'i IKILI ARAMA AGACINA ceviriyor (`cmp #0x38 / bhi`, sonra
 * `cmp #0x18 / bhi` ...), bu yuzden ROM'da 26 farkli deger ve 35 halkalik
 * bir karsilastirma zinciri gorunuyor -- kaynakta bunlarin hepsi duz `case`.
 *
 * KURAL 45 UYGULANDI (docs/COMPILER.md): asagidaki yardimci kalip DOKUZ
 * ayri case icinde geciyor ve ROM'da her biri KENDI havuz kelimesinden
 * 0x020303C4'u yukluyor -- yani dokuz ayri fiziksel blok.  Ortak yerel
 * verilirse agbcc bunlari capraz atlamayla birlestirir ve bloklar kaybolur;
 * o yuzden her case'in KENDI yerelleri var.
 *
 * Not: Ghidra fonksiyonun sonundaki `bx r0`i "cozulemeyen atlama tablosu"
 * sandi.  Degil; `pop {r4,r5,r6}; pop {r0}; bx r0` sirasi sadece void
 * donusun interworking bicimi.
 *
 * Ghidra bu adreste fonksiyon bile tanimlayamamisti: otomatik analiz burayi
 * ARM kipinde cozmeye calisip "bad instruction data" ile birakmis.  TMode
 * yazmaci elle 1 yapilip bolge yeniden sokuldu (tools/ghidra/
 * ExportDecompileBatch.java).
 *
 * DURUM: 1510/1536, 26 bayt kisa.  Karsilastirma sayisi 92/92 TAM ESLESIYOR
 * ve `mov pc,rX` iki tarafta da yok, yani switch dogru bicimde AGAC olarak
 * derleniyor.  Agacin 92 dugumunun 52'si dogru sirada; kok bizde 0x36, ROM'da
 * 0x38, yani case kumesi birkac deger kayik.
 *
 * BELIRLEYICI BULGU -- 0x4005/0x4026/0x4027/0x4028 DE CASE.
 * Ghidra bunlari havuz sabiti gibi gosteriyor (_DAT_080177d0 ve komsulari)
 * ama karsilastirma agacinin dugumleri.  Onlarsiz yazinca case kumesi
 * 1..0x97 arasinda YOGUN kaliyor ve agbcc ATLAMA TABLOSU uretiyor: 1814 bayt
 * (296 FAZLA), 45 karsilastirma, bir `mov pc,r0`.  Dordu eklenince aralik
 * 1..0x4028'e aciliyor, gcc mecburen agac uretiyor: 1510 bayt, 92
 * karsilastirma, sifir `mov pc`.  Tek degisiklik, 304 bayt.
 *
 * 0x0E case'i ROM'da `cmp #14` olarak GORUNMUYOR; agac (0x0D, 0x0F) araligina
 * tek deger kaldigi icin `< 0x0F` ile ayiriyor.  Eksik case sanip cikarmayin.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_state_step.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define STATE_IDLE   0x7FFF
#define REMAP_FIRST  0x3FFF
#define REMAP_BASE   0x4000
#define REMAP_TABLE  0x08CA45CC
#define TABLE_18     0x083429F6
#define TABLE_17     0x083429D8
#define TASK_FLAG    0x400
#define CLEAR_8000   0xFFFF7FFF

typedef struct Visual {
    u8 pad0[0x26];
    u8 mode;                  /* +0x26 */
} Visual;

typedef struct Owner {
    u8 pad0[0x30];
    s8 phase;                 /* +0x30 */
} Owner;

typedef struct Link {
    u8 pad0[0x54];
    s32 ready;                /* +0x54 */
} Link;

typedef struct Task {
    u8 pad0[4];
    Link *link;               /* +0x04 */
    u8 pad8[0x20];
    u32 flags;                /* +0x28 */
} Task;

typedef struct Holder {
    u8 pad0[0x28];
    Task *task;               /* +0x28 */
} Holder;

typedef struct Entity {
    u8 pad0[0x0c];
    u32 flags;                /* +0x0C */
    u8 pad10[4];
    u8 **detail;              /* +0x14 */
    u8 pad18[4];
    u8 **kind;                /* +0x1C */
} Entity;

typedef struct Actor {
    Owner *owner;             /* +0x00 */
    u8 pad4[2];
    s16 prev;                 /* +0x06 */
    u8 tag;                   /* +0x08 */
    u8 pad9;
    s8 sub;                   /* +0x0A */
    u8 pad0b[5];
    s32 speed;                /* +0x10 */
    u8 pad14[0x14];
    u32 state;                /* +0x28 */
    u8 pad2c[4];
    Entity *entity;           /* +0x30 */
    u8 pad34[8];
    Visual *visual;           /* +0x3C */
} Actor;

extern s32 FUN_0803c400(Entity *entity);
extern s32 SelectSlotCD(void);
extern void FUN_0801686c(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_080198e4(Actor *self, s32 a, s32 b, s32 c);
extern void FUN_080426ec(Task *task, s32 a, Entity *entity);
extern s32 GetAnchorUnk34(void);
extern s32 FUN_08023660(Entity *entity);
extern void FUN_08016990(Actor *self, s32 a);
extern void FUN_08017c28(Actor *self);
extern void FUN_08019260(Actor *self);

/* KURAL 45: her case bu kalibi KENDI yerelleriyle yazmali.  Makro her
 * genislemede yeni adlar uretmedigi icin yereller cagri yerinde bildirilir
 * ve makroya isimleriyle verilir. */
/* Makro parametresi `task` OLAMAZ: onislemci onu `->task` uye adinin
 * icinde de degistirir ve `->t` cikar.  Parametre adlari uye adlariyla
 * cakismamali. */
#define NUDGE(NENT, NTSK)                                                   \
    do {                                                                    \
        (NTSK) = ((Holder *)*(u8 **)gRam020303C4)->task;                    \
        if ((NTSK) != 0) {                                                  \
            (NTSK)->flags |= TASK_FLAG;                                     \
            if ((NTSK)->link->ready != 0) FUN_080426ec((NTSK), 10, (NENT)); \
        }                                                                   \
    } while (0)

void FUN_08017628(Actor *self)
{
    s32 slot;
    u32 state;

    if (FUN_0803c400(self->entity) == 0) return;
    slot = SelectSlotCD();
    if (self->visual != 0) self->visual->mode = 0x20;

    state = self->state;
    if (state != STATE_IDLE) {
        if (state > REMAP_FIRST) {
            self->state = *(u16 *)(REMAP_TABLE + (state - REMAP_BASE) * 2);
        }
        state = self->state;
        switch (state) {
        case 0x38: {
            if (self->prev != 0x38 && self->prev != 0x96) self->sub = 2;
            FUN_0801686c(self, 0x38, 3, 0);
            if (self->visual != 0) self->visual->mode = 0x40;
            break;
        }
        case 0x4027:
        case 0x96: {
            if (self->prev != 0x96) self->sub = 2;
            FUN_0801686c(self, 0x96, 3, 0);
            if (self->visual != 0) self->visual->mode = 0x40;
            break;
        }
        case 0x18: {
            u32 v18 = *(u16 *)(TABLE_18 + **(u8 **)&self->entity->kind * 2);
            if (v18 == 0) {
                FUN_0801686c(self, 0x18, 2, 1);
            } else {
                FUN_0801686c(self, 0x6c, 2, 1);
                FUN_080198e4(self, v18 + 8, 1, 0);
            }
            break;
        }
        /* 0x4005/0x4026/0x4027/0x4028 de AYNI switch'in case'leri.  Ghidra
         * bunlari havuz sabiti (_DAT_080177d0 vb.) diye gosteriyor ama
         * karsilastirma agacinin dugumleri.  Onemli: bu dort deger switch'in
         * ARALIGINI 1..0x4028'e genisletiyor; onlarsiz kume yogun kaliyor ve
         * agbcc atlama tablosu uretiyor (ROM'da hic `mov pc,rX` yok). */
        case 0x4005:
        case 0x17: {
            u32 v17 = *(u16 *)(TABLE_17 + **(u8 **)&self->entity->kind * 2);
            if (v17 == 0) {
                FUN_0801686c(self, state, 1, 1);
            } else {
                FUN_0801686c(self, 0x6a, 1, 1);
                FUN_080198e4(self, v17 + 8, 1, 0x13);
            }
            if (self->visual != 0) self->visual->mode = 0x20;
            break;
        }
        case 0x0d: {
            Entity *e0d;
            FUN_0801686c(self, 0x0d, 2, 1);
            if (self->visual != 0) self->visual->mode = 0x40;
            e0d = self->entity;
            if (e0d != 0 && (e0d->flags & 0x8000) != 0) e0d->flags &= CLEAR_8000;
            break;
        }
        case 0x0e: FUN_0801686c(self, 0x0e, 2, 1); goto lit40;
        case 0x0f: FUN_0801686c(self, 0x0f, 2, 1); goto lit40;
        case 0x10: FUN_0801686c(self, 0x10, 3, 1); goto lit40;
        case 0x0c: FUN_0801686c(self, 0x0c, 3, 1); goto lit40;
        case 0x1f: FUN_0801686c(self, 0x1f, 2, 0); goto lit40;
        case 0x4028:
        case 0x97: FUN_0801686c(self, state, 1, 0);
        lit40:
            if (self->visual != 0) self->visual->mode = 0x40;
            break;

        case 0x01: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x87, 1, 0x0f); break; }
        case 0x2b: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x8a, 1, 0x0f); break; }
        case 0x4b: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x91, 1, 0x0f); break; }
        case 0x57: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x97, 1, 0x07); break; }
        case 0x5b: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x9a, 1, 0x0f); break; }
        case 0x66: { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x8d, 1, 0x0f); break; }
        /* ROM bu kumeleri ARALIK olarak siniyor (uc noktalarda `cmp`:
         * 78/81, 47/48, 33/41).  Tek tek case yazmak switch'i yogunlastirip
         * agbcc'yi ATLAMA TABLOSU uretmeye itiyordu; GNU aralik bicimi
         * zinciri seyrek tutuyor. */
        case 0x4e: case 0x51:
                   { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x94, 1, 0x0f); break; }
        case 0x2f: case 0x30:
                   { Entity *e; Task *t; e = self->entity; NUDGE(e, t);
                     FUN_080198e4(self, 0x90, 1, 0x47); break; }

        case 0x02: { Entity *e; Task *t; FUN_0801686c(self, 0x02, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x03: { Entity *e; Task *t; FUN_0801686c(self, 0x03, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x2c: { Entity *e; Task *t; FUN_0801686c(self, 0x2c, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x2d: { Entity *e; Task *t; FUN_0801686c(self, 0x2d, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x5c: { Entity *e; Task *t; FUN_0801686c(self, 0x5c, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x5d: { Entity *e; Task *t; FUN_0801686c(self, 0x5d, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x67: { Entity *e; Task *t; FUN_0801686c(self, 0x67, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }
        case 0x68: { Entity *e; Task *t; FUN_0801686c(self, 0x68, 9, 1);
                     e = self->entity; NUDGE(e, t); break; }

        case 0x21: case 0x29:
        case 0x58:
            FUN_080198e4(self, 0x79, 1, 0x23);
            break;
        case 0x35: FUN_080198e4(self, 0x81, 1, 0x23); break;
        case 0x36: FUN_080198e4(self, 0x84, 1, 0x23); break;

        case 0x4026:
            FUN_0801686c(self, state, 3, 1);
            if (self->visual != 0) self->visual->mode = 0x20;
            break;
        case 0x63: FUN_0801686c(self, 0x63, 10, 2); break;
        case 0x4c: FUN_0801686c(self, 0x4c, 10, 2); break;

        default:
            FUN_0801686c(self, state, 2, 2);
            break;
        }
        self->state = STATE_IDLE;
    }

    if (GetAnchorUnk34() == 0) {
        if ((u8)(self->owner->phase - 2) < 2) {
            if (self->visual != 0) self->visual->mode = 0x40;
            if (self->prev != 0x38) FUN_0801686c(self, 0x20, 3, 0);
        } else if (FUN_08023660(self->entity) == 0
                   || (*(s8 *)(self->entity->detail + 0x114) != 2
                       && (u8)(self->sub - 2) < 2)) {
            if (FUN_08023660(self->entity) == 0) {
                FUN_08017c28(self);
                FUN_08019260(self);
            } else {
                self->tag = 3;
                FUN_0801686c(self, 0x4000, 2, 3);
                if (self->visual != 0) self->visual->mode = 0x20;
            }
        } else {
            if (FUN_0803c400(self->entity) != 0
                && *(s8 *)(self->entity->detail + 0x114) == 2
                && self->speed > 0x30000) {
                self->speed = 0x30000;
            }
            if (self->visual != 0) self->visual->mode = 0x10;
        }
    }
    FUN_08016990(self, 0x10000);
}
