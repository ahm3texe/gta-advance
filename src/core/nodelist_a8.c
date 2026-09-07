/* Secili dugumu birakip kaydin yuvasini serbest isaretleme - 0x08053E9C, 206 bayt.
 *
 * Iki kayit isaretcisi var: 0x02026F34 (kip 2) ve 0x020272C8 (oteki kipler).
 * Fonksiyon once HER ZAMAN 0x020272C8'deki kaydin +0x30 secili dugumunu ve
 * onun +0x24 ekini okuyor; sonra kipe gore ilgili kaydi bosaltip yuvasinin
 * +0x0C bayraklarini duzenliyor, kaydi yuvaya geri baglayip (+0x2C) secili
 * dugum uzerinde bir temizlik yapiyor. Bosaltilan yuva donuyor, basarisizlikta 0.
 *
 * ROM'dan OLCULEN ayrintilar
 * --------------------------
 * - Tek parametre r0'da geliyor ve yalniz `cmp r0,#2` ile sinaniyor.
 * - `pop {r4,r5,r6}; pop {r1}; bx r1` -> r0 canli, yani donus tipi void DEGIL
 *   (kural 35'in tersi). Donen deger r5, yani bosaltilan yuvanin isaretcisi.
 * - Kayit yerlesimi: +0x0B bayrak baytI (1 biti kuruluyor), +0x28 yuva,
 *   +0x30 secili dugum. Yuva yerlesimi: +0x0C bayrak kelimesi (0x40, 0x01,
 *   0x80, 0x10 bitleri), +0x2C kayda geri isaretci.
 * - Dugum yerlesimi: +0x18 bayrak kelimesi (0x400 biti `movs #0x80/lsls #3`
 *   ile kuruluyor), +0x24 ek kayit, +0x28 yuva.
 * - `movs r2,#17; negs r2,r2` -> maske int genisliginde kuruluyor, yani
 *   yuva bayragi u32 (kural 47: dar alan olsaydi 0xEF'e katlanirdi).
 * - 0x08041EE0 argumansiz (data/functions.csv'de 2 bayt, empty_stubs.c'de
 *   `void NoOp08041EE0(void)`); cagridan onceki r0 tesadufen yuvayi tutuyor.
 * - 0x02026F34 ve 0x020272C8 ram_map'te `u32` bildirilmis. Kardes
 *   nodelist_a7.c gibi tur DEGISTIRILMEDI, adres alinip cast edildi --
 *   celiskili extern tur yaratmamak icin.
 *
 * YENIDEN YUKLEMELERIN SEBEBI (olculdu)
 * -------------------------------------
 * agbcc'nin CSE'si isaretci uzerinden yapilan HER saklamada bellek
 * onbellegini bosaltiyor. Bu yuzden ROM `(*taban)`i uc kez okuyor:
 *   - basta (secili dugum icin),
 *   - `+0x30 = 0` saklamasindan sonra (+0x0B baytI icin),
 *   - `+0x0B` saklamasindan sonra (kuyruk icin).
 * Kaynakta bunun karsiligi, kip 2 dalinda pointeri BIR yerelde tutup
 * (agbcc yereli bosaltmaz) kuyruk icin AYRI bir okuma yapmak; oteki dalda
 * ise `(*secondary)` ifadesini dogrudan kullanmak. volatile GEREKMIYOR --
 * a7'deki `Ctx *volatile *` numarasi burada gereksiz, denendi ve fark
 * yaratmadi.
 *
 * DENENIP ELENEN YAZIMLAR
 * -----------------------
 * 1. `primary` adres yerelini fonksiyon BASINDA bildirmek: 206 bayt ama
 *    fark 41. Adres tum fonksiyon boyunca yasayip r7'yi kapiyor ve prolog
 *    `push {r4,r5,r6,r7,lr}` oluyor; ROM'da r7 yok. Kural 16: bildirim yeri
 *    degil ATAMA yeri onemli -- atamayi kip 2 blogunun icine almak r7'yi
 *    kaldirdi (fark 41 -> yapisal olarak dogru dagitim).
 * 2. Tek ortak `u32 f` (iki dalda paylasilan bayrak yereli): dagitim
 *    kaymasi. ROM kip 2 dalinda bayragi r2'de, oteki dalda r1'de tutuyor;
 *    yani IKI AYRI yerel (kural 45). Ayirinca oteki dal birebir oturdu.
 * 3. Tek ortak `ctx` + if/else SONRASINDA tek `ctx->slot->owner = ctx;`
 *    satiri: 202 bayt, 4 KISA. Iki dalin yeniden yukleme komutu da ayni
 *    yazmac ciftine dustugu icin (`ldr r1,[r3,#0]`) agbcc capraz atlamayi
 *    BIR KOMUT ILERI tasiyip yeniden yuklemeyi de birlestirdi; ROM'da o
 *    komut iki kez fiziksel olarak var (kip 2 dalinda `ldr r1,[r1,#0]`,
 *    otekinde `ldr r1,[r3,#0]` -- adres yazmaci farkli oldugu icin ROM'da
 *    birlesemiyor). Kaybolan 2 bayt + havuz hizalama dolgusu 2 bayt = 4.
 * 4. Kuyruk okumasini `*(Ctx *volatile *)&gRam02026F34` yapmak (kural 39):
 *    yine 202. volatile bayragi capraz atlamayi ENGELLEMIYOR.
 * 5. KURAL 45 cozdu: her dala kendi `ctx` yereli verilip
 *    `ctx->slot->owner = ctx;` satiri IKI DALA DA yazildi. Ortak son iki
 *    komut (`ldr r0,[r1,#0x28]; str r1,[r0,#0x2c]`) yine capraz atlamayla
 *    birlesiyor -- ROM'daki `b 0x8053F20` tam olarak o birlesme -- ama
 *    yeniden yukleme her dalda kaldi. Fark 0.
 *
 * ESLESME: 206/206 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a8.c
 */

#include "gba_types.h"

struct Ctx;

/* Yuva; burada +0x0C bayrak kelimesi ve +0x2C geri isaretcisi kullaniliyor. */
typedef struct Slot {
    u8          pad00[0x0C];
    u32         flags;          /* +0x0C */
    u8          pad10[0x1C];
    struct Ctx *owner;          /* +0x2C */
} Slot;

/* Ekin gosterdigi kayit; yalniz +0x1C alani sifir mi diye bakiliyor. */
typedef struct Detail {
    u8    pad00[0x1C];
    void *unk1C;                /* +0x1C */
} Detail;

/* Secili dugumun +0x24 eki. */
typedef struct Extra {
    u8      pad00[0x14];
    Detail *detail;             /* +0x14 */
} Extra;

typedef struct ListNode {
    u8     pad00[0x18];
    u32    flags;               /* +0x18, 0x400 biti sinaniyor */
    u8     pad1C[8];
    Extra *extra;               /* +0x24 */
    Slot  *slot;                /* +0x28 */
} ListNode;

typedef struct Ctx {
    u8        pad00[0x0B];
    u8        dirty;            /* +0x0B, 1 biti kuruluyor */
    u8        pad0C[0x1C];
    Slot     *slot;             /* +0x28 */
    u8        pad2C[4];
    ListNode *sel;              /* +0x30 */
} Ctx;

/* GEREKEN SEMBOL BILDIRIMI (yazilmadi, rapor ediliyor): ikisi de ram_map'te
 * `u32` olarak duruyor ama birer Ctx isaretcisi tutuyorlar. Turu degistirmek
 * src/world/node_search.c ve src/core/nodelist_a7.c ile celiskili extern
 * yaratacagi icin mevcut tur korunup adres alindi. */
extern u32 gRam02026F34;            /* 0x02026F34 */
extern u32 gRam020272C8;            /* 0x020272C8 */

extern s32  FUN_08055888(ListNode *node, s32 mode);
extern void NoOp08041EE0(void);

/* 0x08053E9C */
Slot *ReleaseSelectedNode(s32 kind)
{
    Ctx **secondary = (Ctx **)&gRam020272C8;
    Ctx *active;
    Ctx *ctxPrimary;
    Ctx *ctxSecondary;
    ListNode *sel;
    Extra *extra;
    Slot *slot;
    Slot *held;

    sel = (*secondary)->sel;
    extra = 0;
    if (sel != 0)
        extra = sel->extra;

    if (kind == 2) {
        /* Kural 16: adres atamasi BLOGUN ICINDE; fonksiyon basinda olursa
           omru uzayip r7'yi kapiyor ve prolog buyuyor. */
        Ctx **primary = (Ctx **)&gRam02026F34;
        u32 primaryFlags;

        active = *primary;
        if (active->sel == 0)
            return 0;
        slot = active->slot;
        primaryFlags = slot->flags;
        if (primaryFlags & 0x40)
            return 0;
        if (primaryFlags & 1)
            return 0;
        active->sel = 0;
        slot->flags = primaryFlags | 0x80;
        active->dirty |= 1;
        /* Yukaridaki saklama onbellegi bosaltiyor: ROM burada kaydi
           YENIDEN okuyor. Kural 45: bu yerel dala ozel. */
        ctxPrimary = *primary;
        ctxPrimary->slot->owner = ctxPrimary;
    } else {
        u32 secondaryFlags;

        if (*secondary == 0)
            return 0;
        if (sel == 0)
            return 0;
        slot = (*secondary)->slot;
        secondaryFlags = slot->flags;
        if (secondaryFlags & 0x40)
            return 0;
        (*secondary)->sel = 0;
        if (!(secondaryFlags & 1))
            slot->flags = secondaryFlags | 0x80;
        (*secondary)->dirty |= 1;
        ctxSecondary = *secondary;
        ctxSecondary->slot->owner = ctxSecondary;
    }

    if (sel->slot != 0) {
        if ((sel->flags & 0x400)
         || (extra != 0 && extra->detail != 0 && extra->detail->unk1C != 0)) {
            if (FUN_08055888(sel, 0) == 0) {
                held = sel->slot;
                held->flags &= ~0x10;
                NoOp08041EE0();
            }
        }
    }
    return slot;
}
