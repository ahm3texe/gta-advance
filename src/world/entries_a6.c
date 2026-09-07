/* Giris animasyonunu bir kare ilerletme -- 0x080289BC-0x08028A87
 *
 * gEntriesA girisinin (148 bayt, kardes src/world/entries_a5.c) animasyon
 * konumunu ilerletiyor. Akis:
 *
 *   node  = gRom08BD3448.slots[e->kind]->slots[e->phase]
 *   limit = node->count << 16              (16.16 sabit noktali son kare)
 *   konum limit'e ulastiysa:
 *       phase != 51 -> FUN_08013ABC(e->sub) ile alt nesne birakilir,
 *                      e->active = 0 ve DONULUR
 *       phase == 51 -> konum limit - 0x20000'e geri sarilir (iki kare geri)
 *   frame = node->slots[konumun UST yarim sozu]
 *   FUN_08013CFC(e->sub, frame, 0);  FUN_08014EE4(e->sub, frame->unk14);
 *   phase == 51 ise konum += 0x5000, tasarsa (0x4FFFF'i gecerse)
 *       FUN_08035230(GetActiveSlot(), 469) cagrilir ve sahip nesnenin
 *       +0x0A bayragi 2 ise +0x0C'ye 0x8000 bayragi eklenir
 *   degilse konum += 0x8000
 *
 * ROM'dan OLCULEN AYRINTILAR
 * -------------------------
 * Kural 35: `pop {r4,r5,r6,r7}; pop {r0}; bx r0` -> donus tipi void.
 *
 * KONUM ALANI (+0x8C) BIR BIRLESIM (union). ROM ayni dort bayti iki
 * genislikte okuyor: kelime olarak (`ldr r0,[r7,#0]`, `str`) ve UST yarim
 * sozu ISARETLI olarak (`movs r1,#2; ldrsh r0,[r7,r1]`). Thumb'da LDRSH'in
 * ancak yazmac-ofsetli bicimi var, bu yuzden taban `e+0x8C`, indis 2
 * olarak kuruluyor. `konum >> 16` yazmak `asrs` uretirdi -- ROM'da yok,
 * yani kaynakta gercekten ayri bir s16 alan var.
 *
 * IKI KARSILASTIRMANIN ISARETLILIGI FARKLI, bu tipleri belirledi:
 *   0x80289EA `bcc`  -> ISARETSIZ  : `konum >= limit`, limit `u32` yerel
 *   0x8028A42 `ble`  -> ISARETLI   : `konum > 0x4FFFF`, alan `s32`
 * Yani alan s32; ust sinir karsilastirmasini isaretsize ceviren sey
 * `limit`in u32 olmasi. limit'i `int` yapmak `blt` uretiyor, alani u32
 * yapmak ikinci dali `bls` yapiyor -- ikisi de yanlis.
 *
 * PHASE UC KEZ OKUNUYOR AMA IKI YUKLEME VAR. Ilk iki kullanim (dizi indisi
 * ve `cmp #51`) tek `ldr`den geliyor, ucuncusu (0x8028A2E) cagrilardan
 * SONRA yeniden yukleniyor. Kaynakta her yerde `e->phase` yazmak tam bunu
 * uretiyor: agbcc'nin CSE'si cagri sinirinda bellek okumasini gecersiz
 * kiliyor. Ayri bir `phase` yereli tutmak degeri callee-saved yazmaca
 * tasitip fazladan bir push isterdi.
 *
 * Kural 33 (maske sabiti kendi yazmacinda): ROM `movs r0,#2` komutunu
 * `ldrh r1,[r5,#10]`den ONCE veriyor ve sonucu sabitin yazmacinda tutuyor.
 * `if ((owner->flags & 2) != 0)` yazimi ldrh'i one aliyor; `mask = 2;
 * mask &= owner->flags;` ROM sirasini veriyor.
 *
 * Kural 49 dogrulamasi: ROM'da "bitir" govdesi (FUN_08013ABC + active=0)
 * kosulun DUZ dalinda, geri sarma govdesi ise havuzdan sonra ve L_a06'ya
 * DUSEREK duruyor. Bu, duz `if (...) { if (phase != 51) { ...; return; }
 * geri-sar; }` yaziminin dogal yerlesimi; goto/etiket gerekmedi.
 *
 * Bu fonksiyon gEntriesA'ya HIC dokunmuyor (girisi parametre olarak
 * aliyor), o yuzden tablo sembolu bildirilmedi.
 *
 * SON 8 BAYT NASIL KAPANDI (212 -> 204, fark 100 -> 0)
 * ---------------------------------------------------
 * Ilk surumde her yerde `e->pos...` yazilmisti. Govde komut komut ROM ile
 * ayniydi ama prolog `push {r4,r5,r6,lr}` cikiyordu, ROM'unki
 * `push {r4,r5,r6,r7,lr}`. Yani ROM'da BIR CANLI DEGER FAZLA var
 * (docs/COMPILER.md register tablosu): ROM `e+0x8C` adresini r7'de tutup
 * UC KEZ (`ldrsh`, phase 51 adimi, varsayilan adim) yeniden kullaniyor,
 * bizimki her seferinde `adds r0,r6,#0 / adds r0,#140` ile yeniden
 * kuruyordu -- toplam ucer komut fazla, 8 bayt.
 *
 * COZUM: `Pos *pos = &e->pos;` yerelini acmak. Kural 11/16'nin birebir
 * ornegi: onemli olan bildirim degil ATAMA YERI. Atama fonksiyonun basina
 * konursa omur uzuyor ve ILK blok da r7'yi kullaniyor; ROM ise ilk blokta
 * adresi scratch r2'de kurup L_a06'da r7'ye YENIDEN kuruyor. Bu yuzden
 * atama tam olarak geri-sarma if'inden SONRA, ilk `ldrsh` kullanimindan
 * hemen once duruyor -- ROM'daki `adds r7,r6,#0 / adds r7,#140` cifti
 * oraya oturuyor. Ilk blok yerelden once oldugu icin orada hala dogrudan
 * `e->pos.value` yazili; ikisi bilerek karisik.
 *
 * DENENIP ELENENLER
 * -----------------
 * - `e->pos.value` her yerde (tek yerel yok) -> 212 bayt, fark 100.
 *   Govde dogru, tek eksik r7'nin canli kalmasi.
 * - `pos` yerelini fonksiyonun basinda atamak DENENMEDI ama kural 16
 *   geregi ilk blogun r2 yerine r7 kullanmasina yol acar; ROM'da ilk
 *   blok r2 kullaniyor, yani bu yol ROM'a AYKIRI.
 * - `konum >> 16` ile kare indisi: ROM'da `asrs` yok, `ldrsh` var; bu
 *   yuzden birlesim (union) alani yazildi, kaydirma denenmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_a6.c
 */

#include "gba_types.h"

#define PHASE_SPECIAL   51          /* +0x90 == 51 ayrik dal */
#define REWIND_STEP     0x20000     /* geri sarmada iki kare */
#define STEP_SPECIAL    0x5000      /* phase 51 kare adimi */
#define STEP_DEFAULT    0x8000      /* diger phase'lerde kare adimi */
#define END_LIMIT       0x0004FFFF  /* phase 51'de tasma esigi */
#define NOTIFY_ID       469         /* FUN_08035230 ikinci argumani */
#define OWNER_FLAG      2           /* +0x0A icinde sinanan bit */
#define OWNER_SET_BIT   0x8000      /* +0x0C'ye eklenen bit */

/* 0x08BD3448'deki ROM koku ve ondan zincirlenen dugumler; src/world/
 * entries_b1.c'deki RomNode ile ayni yerlesim. Fark: bu ceviri birimi
 * +0x00'i BAYT olarak okuyor (`ldrb`), orada dolguydu. */
typedef struct RomNode {
    u8              count;      /* +0x00, kare sayisi */
    u8              pad01[3];
    struct RomNode **slots;     /* +0x04 */
    u8              pad08[12];
    u32             unk14;      /* +0x14 */
} RomNode;

extern RomNode gRom08BD3448;

/* +0x8C: 16.16 sabit noktali animasyon konumu. Kelime olarak da, ust
 * yarim sozu ISARETLI olarak da okunuyor. */
typedef struct PosHalf {
    u16 frac;                   /* +0x00 */
    s16 frame;                  /* +0x02 */
} PosHalf;

typedef union Pos {
    s32     value;
    PosHalf half;
} Pos;

/* +0x84'teki sahip nesnesi; yalnizca iki alani okunuyor. */
typedef struct Owner {
    u8  pad00[10];
    u16 flags;                  /* +0x0A */
    u32 bits;                   /* +0x0C */
} Owner;

/* Kardes dosyalarla (entries_a1.c ... entries_a5.c, entries_b1.c) ayni
 * yerlesim; bu fonksiyonun dokundugu alanlar acildi. Toplam 148 = 0x94. */
typedef struct Entry {
    u8     active;              /* +0x00 */
    u8     pad01[3];
    u8     sub[38];             /* +0x04, FUN_08013CFC/FUN_08014EE4'e verilir */
    u8     pad2a[58];
    u8     kind;                /* +0x64 */
    u8     pad65[31];
    Owner *owner;               /* +0x84 */
    u8     pad88[4];
    Pos    pos;                 /* +0x8C */
    u32    phase;               /* +0x90, stride 148 */
} Entry;

extern void FUN_08013abc(u8 *sub);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, u32 value);
extern u32 GetActiveSlot(void);
extern void FUN_08035230(u32 slot, u32 id);

/* 0x080289BC */
void AdvanceEntryFrame(Entry *e)
{
    RomNode *node;
    RomNode *frame;
    Owner *owner;
    Pos *pos;
    u32 limit;
    u32 mask;

    node = gRom08BD3448.slots[e->kind]->slots[e->phase];
    limit = node->count << 16;

    if (e->pos.value >= limit) {
        if (e->phase != PHASE_SPECIAL) {
            FUN_08013abc(e->sub);
            e->active = 0;
            return;
        }
        e->pos.value = limit - REWIND_STEP;
    }

    pos = &e->pos;
    frame = node->slots[pos->half.frame];
    FUN_08013cfc(e->sub, frame, 0);
    FUN_08014ee4(e->sub, frame->unk14);

    if (e->phase == PHASE_SPECIAL) {
        pos->value += STEP_SPECIAL;
        if (pos->value > END_LIMIT) {
            FUN_08035230(GetActiveSlot(), NOTIFY_ID);
            owner = e->owner;
            mask = OWNER_FLAG;
            mask &= owner->flags;
            if (mask != 0)
                owner->bits |= OWNER_SET_BIT;
        }
    } else {
        pos->value += STEP_DEFAULT;
    }
}
