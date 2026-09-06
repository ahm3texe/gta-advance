/* Aktore yeni bir kimlik yukluyor; 12/13/15 ve 16 kimliklerinde bagli
 * yapinin sahibindeki kip baytini isaretliyor.
 * 0x0801686C, 292 bayt.  BYTE-MATCHING.
 *
 * AKIS
 *   0x7FFF nobetci deger; kimlik 0x3FFF ustundeyse aktorun +0x38'deki
 *   cevrim tablosundan (id - 0x4000) girdisi okunup gercek kimlik
 *   bulunuyor.  Kimlik zaten yuruyorsa yalnizca varliga dokunulup
 *   cikiliyor.  Aksi halde sayac sifirlaniyor, kimlik/istek/yuva/rutbe
 *   yaziliyor, durum sifirlaniyor ve iki kimlik ailesi icin bagli
 *   yapinin sahibi isaretleniyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_b2.c
 *
 * OLCULEN UC KALIP (hepsi tek tek dogrulandi, degistirmeyin):
 *
 * 1) `t` GECICISI SART (kural 40).  ROM birlesme noktasinda iki kopya
 *    tutuyor: else dali `adds r0,r1,#0`, birlesme `adds r2,r0,#0`.
 *    `t` atilip dogrudan `value`ya yazilinca iki dal da value'nun
 *    yazmacina uretiyor, iki komut birden dusuyor (280/292).
 *
 * 2) TABLO TESTI IC ICE IKI `if` OLMALI, `&&` DEGIL.  Iki ayri `t = id`
 *    yazimi gerekiyor: yeniden yerlestirme sonrasi ikisi capraz-atlama
 *    ile tek bloga kaynasiyor, yani URETILEN KOD AYNI; degisen tek sey
 *    `id`nin basvuru sayisi 11 -> 12.  Kural 50 onceligi bununla
 *    0.493'ten 0.522'ye cikip `value`nun 0.500'unu geciyor ve dagitim
 *    ROM'unki oluyor (id r1, value r2, slot r5).  `&&` yazimiyla sira
 *    tersine donuyor: id r5, value r1 -- govdenin yarisi kayiyor.
 *    Olcum: python3 tools/dump_alloc.py src/world/actor_b2.c FUN_0801686c
 *
 * 3) BAGLI YAPI KUYRUKTA YENIDEN OKUNUYOR.  ROM, `+0xAC` ADRESINI r3'te
 *    saklayip birlesme noktasinda isaretciyi TEKRAR yukluyor
 *    (`ldr r0,[r3,#0]`), oysa agbcc'nin CSE'si iki dalin birlestigi
 *    yerde bile yuklu degeri elde tutuyor.  Iki parca gerekiyor:
 *      - okuma `volatile` uzerinden yapilmali ki CSE elenmesin;
 *      - adres AYRI bir yerelde (`slotp`) tutulmali ve o yerel
 *        FONKSIYON KAPSAMINDA olmali.  Blok kapsaminda tek tanimi olunca
 *        local-alloc kopyayi eleyip adresi tek yazmaca indiriyor ve
 *        `adds r3,r0,#0` kayboluyor (288/292).  Iki case'te iki tanim
 *        olunca kopya ayakta kaliyor.
 *
 * ELENEN YOLLAR (tekrar denemeyin):
 *   - `value`yu dogrudan if/else'te uretmek           -> 280, 62/144
 *   - dal basina ayri yerel (`mapped`/`plain`)        -> value 5 basvuru,
 *                                                       oncelik 0.556, ters dagitim
 *   - `flags` yerelini atip `self->link->flags`       -> fark yok
 *   - `Link **held = &self->link;` (volatile'siz)     -> CSE yine eliyor
 *   - kuyrukta `owner` adli fonksiyon kapsamli yerel  -> adres kopyasi
 *                                                       fazladan cikiyor, 114/144
 *   - `mask` fonksiyon kapsamli yerel                 -> maske r2'ye dusuyor,
 *                                                       ROM r0 kullaniyor
 *   - iki ayri volatile okuma (yukle+sakla)           -> 302 bayt
 */

#include "gba_types.h"

/* Aktorun +0x30'daki varligi; burada yalnizca adresi gecirildigi icin
 * ic yerlesimi acilmadi (kardes: src/world/actor_reset.c). */
typedef struct Entity {
    u8 pad0[4];
} Entity;

typedef struct Holder {
    u8 pad0[0x30];
    u8 kind;                  /* +0x30 */
} Holder;

typedef struct Owner {
    u8 pad0[0xa8];
    u8 mode;                  /* +0xA8 */
} Owner;

typedef struct Link {
    u8 pad0[0x0c];
    u32 flags;                /* +0x0C */
    u8 pad10[8];
    Holder *holder;           /* +0x18 */
    Owner *owner;             /* +0x1C */
} Link;

typedef struct Actor {
    u8 pad0[4];
    u16 current;              /* +0x04 */
    u16 request;              /* +0x06 */
    u8 rank;                  /* +0x08 */
    u8 slot;                  /* +0x09 */
    u8 state;                 /* +0x0A */
    u8 pad0b[5];
    u32 timer;                /* +0x10 */
    u8 pad14[0x1c];
    Entity *entity;           /* +0x30 */
    u8 pad34[4];
    u16 *table;               /* +0x38 */
    u8 pad3c[0x70];
    Link *link;               /* +0xAC */
} Actor;

extern void GetOwnerSlot(Entity *entity);

void FUN_0801686c(Actor *self, u32 id, u32 slot, u32 rank)
{
    u32 value;                /* cevrilmis kimlik */
    u32 flags;                /* bagli yapinin +0x0C bayraklari */
    Link *lnk;                /* bagli yapi, kapi testleri icin */
    /* Kuyruktaki yeniden okumanin adresi.  Fonksiyon kapsaminda ve
     * volatile: gerekcesi dosya basligindaki (3) numarali kalip. */
    Link *volatile *slotp;

    if (id == 0x7fff) return;                 /* nobetci kimlik */
    /* Rutbe karsilastirmasi ISARETSIZ (bcs); kural 31. */
    if (self->rank < rank && self->state != 2) return;
    if (rank == 0 && self->rank == 0 && self->state != 2) return;

    /* Kimlik cevrimi.  Ic ice iki `if` ve iki ayri `t = id` yazimi
     * zorunlu; gerekcesi basliktaki (1) ve (2) numarali kaliplar. */
    {
        u32 t;
        if (self->table != 0) {
            if (id > 0x3fff)
                t = self->table[id - 0x4000];   /* girdi 0x4000 kaydirilmis */
            else
                t = id;
        } else {
            t = id;
        }
        value = t;
    }

    if (value == 0x7fff) return;              /* cevrim de nobetci verdi */
    /* Kimlik zaten yuruyor: varliga haber verilip cikiliyor. */
    if (self->current == value && self->state != 2) {
        GetOwnerSlot(self->entity);
        return;
    }

    /* Yazma sirasi ROM'un: sayac, kimlik, istek, yuva, rutbe, durum. */
    self->timer = 0;
    self->current = value;
    self->request = id;
    self->slot = slot;
    self->rank = rank;
    self->state = 0;

    switch (id) {
    case 12:
    case 13:
    case 15:
        /* Adres once yakalaniyor, isaretci sonra tekrar okunuyor. */
        lnk = self->link;
        slotp = &self->link;
        if (lnk == 0) return;
        flags = lnk->flags;
        /* 0x80 biti kapiyi dogrudan aciyor; degilse ucu de tutmali. */
        if ((flags & 0x80) == 0) {
            if ((flags & 1) == 0) return;
            if ((flags & 0x800) != 0) return;
            if (lnk->holder->kind == 2) return;
        }
        {
            /* `own` blok kapsaminda: fonksiyon kapsamli yerel adres
             * kopyasini fazladan uretiyor (basliktaki elenen yollar). */
            Owner *own = (*slotp)->owner;
            own->mode = (own->mode & 0x3f) | 0x80;   /* alt 6 bit korunuyor */
        }
        break;
    case 16:
        /* Adres once yakalaniyor, isaretci sonra tekrar okunuyor. */
        lnk = self->link;
        slotp = &self->link;
        if (lnk == 0) return;
        flags = lnk->flags;
        /* 0x80 biti kapiyi dogrudan aciyor; degilse ucu de tutmali. */
        if ((flags & 0x80) == 0) {
            if ((flags & 1) == 0) return;
            if ((flags & 0x800) != 0) return;
            if (lnk->holder->kind == 2) return;
        }
        {
            /* `own` blok kapsaminda: fonksiyon kapsamli yerel adres
             * kopyasini fazladan uretiyor (basliktaki elenen yollar). */
            Owner *own = (*slotp)->owner;
            own->mode = (own->mode & 0x3f) | 0x40;   /* alt 6 bit korunuyor */
        }
        break;
    }
}
