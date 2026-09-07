/* FUN_0803afbc — 0x0803AFBC-0x0803B19B (480 bayt)
 *
 * ESLESIYOR (byte-matching, 480/480 bayt, 231/231 komut ayni).
 *
 * NE YAPIYOR: gRam02000F08'in gosterdigi baglam icin "sahne gecisi"
 * adimi.  Once bes kapi var (bayrak +0xAE bit 1, mesgul bayragi +0x08 ve
 * uc FUN_08019620 sorgusu 0x400C / 0x4009 / 0x400B); herhangi biri
 * tutarsa hicbir sey yapmadan cikiyor.  Ardindan dugumun (+0x1C)
 * durumuna gore bir bekleme esigi seciyor (durum 8/9 -> 18, 10 -> 24) ve
 * +0xB8'deki adim sayaci sifirdan buyuk ama esigin altindaysa yine
 * cikiyor.  Kapilar gecilince: dugumun +0x02 bayragi hala sifirsa
 * GetRamType()'a bakip bayragi kuruyor; kurulamadiysa duruma gore
 * FUN_08035058 ile 348 ya da 349 bildirimini yollayip cikiyor.  Bayrak
 * kuruluysa iki yol var: durum 13 ise +0xB4'teki nesnenin konumunu
 * (bayrak 0x30'a gore +0x18 ya da +0x20'nin +4'u) baglamin +0x10'una 12
 * bayt olarak kopyalayip etkin yuvayla FUN_08035f1c'i cagiriyor; degilse
 * evreyi 2 yapip +0xBC'yi sifirliyor, dugumun dizideki indeksini
 * ((node - slots) >> 3) ve oyuncu hizinin mutlak degerini FUN_0803a554'e
 * verip, gRam02000F00 == gSlotSelector + 1 ise FUN_08030a3c'yi cagiriyor
 * ve son olarak +0x06 zamanlayicisini 900, +0xB8 adimini 1 yapiyor.
 *
 * ---------------------------------------------------------------------
 * ROM'DAN OKUNAN AYRINTILAR
 *
 *  1. `node` (+0x1C) YEREL DEGISKEN, geri kalan her sey global uzerinden
 *     okunuyor.  0x803AFC2'de `ldr r5,[r2,#28]` ile en basta aliniyor ve
 *     r5 uc `bl`'yi asarak 0x803B176'ya kadar yasiyor.  Cagrilar
 *     bellegi bozdugu icin bu ancak GERCEK bir yerelse mumkun; buna
 *     karsilik durum baytini okuyan uc yer (0x803B00E, 0x803B06A,
 *     0x803B0C8) her seferinde `gRam02000F08->node->state` zincirini
 *     bastan yukluyor.  Iki yazim bilerek ayri tutuldu: `node->state`
 *     yazilirsa r5 kullanilir ve uc `ldr` cifti kaybolur.
 *
 *  2. ESIK SECIMI SWITCH, if/else DEGIL.  0x803B014'te `cmp #8 / blt`,
 *     `cmp #9 / ble`, `cmp #10 / bne` var: bu agbcc'nin seyrek switch
 *     karar agaci (kural 46).  `if (s == 8 || s == 9)` yazimi kural 60
 *     geregi aralik testine (`subs #8 / cmp #1 / bhi`) katlanirdi.
 *     `default:` dogrudan `goto ready;` -- ROM'un `blt L_b050`'si bu.
 *     CASE SIRASI onemli: ROM'da `movs r2,#24` govdesi agacin hemen
 *     ardinda (duse gecisle), `movs r2,#18` govdesi ortak koddan hemen
 *     once duruyor; yani kaynakta once `case 10`, sonra `case 8/9`.
 *
 *  3. +0xB8 IKI KERE OKUNUYOR (kural 55).  `ldrb r0,[r1,#0] / cmp #0 /
 *     beq` ve hemen ardindan yine `ldrb r1,[r1,#0] / cmp r1,r2 / bge`.
 *     Yerele kopyalamak bu ikinci `ldrb`'yi siler.  Adres (p + 0xB8)
 *     ise CSE ile paylasiliyor -- ikisi de tek `gRam02000F08->step`
 *     yazimindan cikiyor.
 *
 *  4. ASIL SWITCH ATLAMA TABLOSU URETIYOR: durum - 3, `cmp #8 / bls`,
 *     `lsls #2` ve 0x0803B08C'deki 9 sozcukluk tablo.  Tablo bu
 *     fonksiyonun kendi literal havuzunun icinde, veri sembolu degil.
 *     Case 8 ve 9 kaynakta YOK; tablodaki karsiliklari default etiketine
 *     (0x803B18A) bakiyor -- tablo min..max araligini doldurdugu icin.
 *     Case 3/5 govdesi ROM'da AYRI DURMUYOR: agbcc'nin capraz atlamasi
 *     onu 0x803B0DC'deki ozdes blokla (item == 0 dalindaki ayni
 *     FUN_08035058(..., 349) cagrisi) birlestirip tablo girdilerini
 *     dogrudan oraya yonlendirmis.  Kaynakta iki cagri ayri ayri
 *     yazili; birlestirme derleyicinin isi.
 *
 *  5. INDEKS HESABI: ROM `subs r0,#36 / subs r0,r0,r1 / lsrs r2,r0,#3`.
 *     Sabitin ONCE cikarilmasi, gcc'nin `A - (B + sabit)` -> `(A -
 *     sabit) - B` katlamasindan geliyor; yani kaynak
 *     `((u32)node - (u32)slots) >> 3` (slots dizisi +0x24).  `lsrs`
 *     ISARETSIZ, bu yuzden isaretci farki (`node - slots`, ptrdiff_t)
 *     KULLANILAMAZ -- o `asrs` uretirdi.
 *
 *  6. 0x803B112'deki blok GetActiveSlotValue (0x0803C090,
 *     src/world/slot_config.c) ile BIREBIR ayni ama `bl` yok: kaynakta
 *     ayni secim elle yazilmis.  Bu yuzden burada da acik yazildi.
 *
 *  7. FUN_08035f1c TEK ARGUMANLI (0x08035F1C r1'i hic okumuyor).
 *     Cagri oncesi r1 = p + 0x1C degeri, 12 baytlik `ldmia/stmia`
 *     kopyasinin geride biraktigi artik; kaynakta ikinci arguman YOK.
 *
 *  8. `ldrh` (isaretsiz) => +0x02 alani u16; `ldrsh` (0x803B16E) =>
 *     gSlotSelector s16; `blt/ble/bge` isaretli dallar => u8 alanlar
 *     int'e yukseliyor, karsilastirma int sabitleriyle.
 *
 * ---------------------------------------------------------------------
 * KULLANILAN SEMBOLLER (hepsi data/ram_map.csv'de kayitli)
 *   0x02000F08 gRam02000F08  — baglam isaretcisi (Ctx *)
 *   0x02000F04 gSessionPtr   — yuva isaretcisi (Slot *)
 *   0x02000F10 gRam02000F10  — birincil yuva govdesi
 *   0x02000CE0 gGameState    — [12] ikincil yuva secici
 *   0x02000D40 gSlotSelector — s16
 *   0x02000F00 gRam02000F00  — u8
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/band_0803afbc.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define QUERY_A   0x400C
#define QUERY_B   0x4009
#define QUERY_C   0x400B

#define NOTIFY_A  348
#define NOTIFY_B  349

#define STATE_READY  13

#define PHASE_ABORT  2
#define TIMER_RESET  900

typedef struct Vec3 {
    u32 x;
    u32 y;
    u32 z;
} Vec3;

/* +0x1C'deki dugum */
typedef struct Node {
    u8  state;                  /* +0x00 */
    u8  pad01[1];
    u16 pending;                /* +0x02 */
} Node;

/* +0x20'deki isaretcinin hedefi: konum +0x04'te */
typedef struct AltBody {
    u32  pad00;
    Vec3 pos;                   /* +0x04 */
} AltBody;

/* Ctx->item (+0xB4) */
typedef struct Item {
    u8       pad00[8];
    u8       flags;             /* +0x08 */
    u8       pad09[15];
    Vec3    *pos;               /* +0x18 */
    u8       pad1c[4];
    AltBody *alt;               /* +0x20 */
} Item;

/* Ctx->slots dizisinin elemani; adim 8 (ROM: fark >> 3) */
typedef struct NodeSlot {
    u8 raw[8];
} NodeSlot;

typedef struct Ctx {
    u8       pad00[6];
    u16      timer;             /* +0x06 */
    u8       busy;              /* +0x08 */
    u8       pad09[2];
    u8       phase;             /* +0x0B */
    u8       pad0c[4];
    Vec3     pos;               /* +0x10 */
    Node    *node;              /* +0x1C */
    u8       pad20[4];
    NodeSlot slots[17];         /* +0x24 */
    u8       padac[2];
    u16      flags;             /* +0xAE */
    u8       padb0[4];
    Item    *item;              /* +0xB4 */
    u8       step;              /* +0xB8 */
    u8       padb9[3];
    u32      unkBC;             /* +0xBC */
} Ctx;

typedef struct Motion {
    u8  pad00[24];
    s32 speed;                  /* +0x18 */
} Motion;

typedef struct Entity {
    u8      pad00[24];
    Motion *motion;             /* +0x18 */
} Entity;

typedef struct Slot {
    Entity *entry;              /* +0x00 */
} Slot;

extern Ctx  *gRam02000F08;
extern Slot *gSessionPtr;
extern u8    gGameState[];
extern u8    gRam02000F00;
extern s16   gSlotSelector;

extern u32  FUN_08019620(Entity *entry, u32 query);
extern void FUN_08035058(Entity *entry, u32 id);
extern void FUN_08035f1c(Entity *entry);
extern void FUN_0803a554(u32 index, s32 speed, u32 arg3);
extern void FUN_08030a3c(u16 value);
extern u32  GetRamType(void);

/* 0x0803AFBC */
void FUN_0803afbc(void)
{
    Node    *node;
    Item    *item;
    Vec3    *src;
    Entity  *entry;
    u32      index;
    s32      speed;
    int      limit;

    node = gRam02000F08->node;
    if ((gRam02000F08->flags & 2) != 0)
        return;
    if (gRam02000F08->busy != 0)
        return;
    if (FUN_08019620(gSessionPtr->entry, QUERY_A) != 0)
        return;
    if (FUN_08019620(gSessionPtr->entry, QUERY_B) != 0)
        return;
    if (FUN_08019620(gSessionPtr->entry, QUERY_C) != 0)
        return;

    switch (gRam02000F08->node->state) {
    case 10:
        limit = 24;
        break;
    case 8:
    case 9:
        limit = 18;
        break;
    default:
        goto ready;
    }

    if (gRam02000F08->step != 0 && gRam02000F08->step < limit)
        return;

ready:
    if (node->pending == 0) {
        if (GetRamType() != 0)
            node->pending = 1;

        if (node->pending == 0) {
            switch (gRam02000F08->node->state) {
            case 3:
            case 5:
                FUN_08035058(gSessionPtr->entry, NOTIFY_B);
                return;
            case 4:
            case 6:
            case 7:
            case 10:
            case 11:
                FUN_08035058(gSessionPtr->entry, NOTIFY_A);
                return;
            }
            return;
        }
    }

    if (gRam02000F08->node->state == STATE_READY) {
        item = gRam02000F08->item;
        if (item == 0) {
            FUN_08035058(gSessionPtr->entry, NOTIFY_B);
            return;
        }

        if ((item->flags & 0x30) != 0)
            src = &item->alt->pos;
        else
            src = item->pos;

        gRam02000F08->pos = *src;

        if (gGameState[12] == 0)
            entry = ((Slot *)gRam02000F10)->entry;
        else
            entry = gSessionPtr->entry;

        FUN_08035f1c(entry);
    } else {
        gRam02000F08->phase = PHASE_ABORT;
        gRam02000F08->unkBC = 0;
        index = ((u32)gRam02000F08->node - (u32)gRam02000F08->slots) >> 3;
        speed = gSessionPtr->entry->motion->speed;
        if (speed < 0)
            speed = -speed;
        FUN_0803a554(index, speed, 0);
        if (gRam02000F00 == gSlotSelector + 1)
            FUN_08030a3c(node->pending);
        gRam02000F08->timer = TIMER_RESET;
        gRam02000F08->step = 1;
    }
}
